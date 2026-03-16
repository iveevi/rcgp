#pragma once

#include <variant>

#include "../rhi/device.hpp"
#include "../rhi/descriptor_pool.hpp"
#include "../util/cti.hpp"
#include "process_descriptors.hpp"
#include "resources.hpp"
#include "static_string.hpp"
#include "witnesses.hpp"

namespace rcgp {

template <auto &ref>
struct UnboundDescriptor : vk::DescriptorSet {
	using vk::DescriptorSet::DescriptorSet;
};

template <auto &ref>
struct BoundDescriptor : vk::DescriptorSet {
	using vk::DescriptorSet::DescriptorSet;
};

// Counting the number of bindings needed by a resource
template <typename T>
constexpr size_t number_of_bindings = [] constexpr {
	if constexpr (is_resource_group_v <T>) {
		return number_of_bindings <typename T::struct_type>;
	} else if constexpr (user_defined <T>) {
		// TODO: need to iterate through the fields themself
		return T::field_count;
	} else {
		return 1;
	}
} ();

// Temporary storage for descriptor infos
struct descriptor_info_union : variant <
	vk::DescriptorImageInfo,
	vk::DescriptorBufferInfo,
	std::vector <vk::DescriptorImageInfo>,
	vk::WriteDescriptorSetAccelerationStructureKHR
> {
	using variant_self::variant;

	auto &set_image(const vk::DescriptorImageInfo &image) {
		*this = image;
		return as <vk::DescriptorImageInfo> ();
	}

	auto &set_image_list(const std::vector <vk::DescriptorImageInfo> &images) {
		*this = images;
		return as <std::vector <vk::DescriptorImageInfo>> ();
	}
	
	auto &set_buffer(const vk::DescriptorBufferInfo &buffer) {
		*this = buffer;
		return as <vk::DescriptorBufferInfo> ();
	}
	
	auto &set_tlas(const vk::WriteDescriptorSetAccelerationStructureKHR &tlas) {
		*this = tlas;
		return as <vk::WriteDescriptorSetAccelerationStructureKHR> ();
	}
};

template <typename T>
auto get_descriptor_info(const auto &resource)
{
	if constexpr (std::is_same_v <T, RaytracingAccelerationStructure>) {
		return vk::WriteDescriptorSetAccelerationStructureKHR()
			.setAccelerationStructures(resource);
	} else if constexpr (is_resource_array_v <T>) {
		using R = T::base;
		return constexpr_for(Is, T::elements,
			return std::vector {
				get_descriptor_info <R> (resource[Is])...
			}
		);
	} else {
		return resource.descriptor_info();
	}
}

template <typename T, size_t I>
void set_descriptor_write(
	const auto &resource,
	const vk::DescriptorSet &set,
	vk::WriteDescriptorSet &write,
	descriptor_info_union &info
)
{
	using enum vk::DescriptorType;

	write // ...
		.setDstSet(set)
		.setDescriptorType(resource_packet <T> ().type)
		.setDstBinding(I);

	auto dinfo = get_descriptor_info <T> (resource);
	if constexpr (is_sampler_v <T> or is_storage_image_v <T>) {
		write.setImageInfo(info.set_image(dinfo));
	} else if constexpr (is_storage_buffer_v <T> or is_uniform_buffer_v <T>) {
		write.setBufferInfo(info.set_buffer(dinfo));
	} else if constexpr (std::is_same_v <T, RaytracingAccelerationStructure>) {
		write // ...
			.setDescriptorCount(1)
			.setPNext(&(info.set_tlas(dinfo)));
	} else if constexpr (is_resource_array_v <T>) {
		using R = T::base;

		if constexpr (is_sampler_v <R> or is_storage_image_v <R>) {
			write.setImageInfo(info.set_image_list(dinfo));
		} else {
			static_assert(false, "unsupported resource array element "_ss + $ss_type(R));
		}
	} else {
		static_assert(false, "unsupported resource type "_ss + $ss_type(T));
	}
}

// Descriptor write handler with temporary storage
template <auto &ref>
struct DescriptorWrite {
	using Reference = reference_base_of <ref>;
	using Descriptor = std::variant <
		UnboundDescriptor <ref>,
		BoundDescriptor <ref>
	>;

	Descriptor descriptor;
	const ResourceTypeFor <ref> &resource;

	static constexpr size_t bindings = number_of_bindings <Reference>;
	std::array <descriptor_info_union, bindings> info_unions;

	[[nodiscard]] auto descriptor_handle() const -> vk::DescriptorSet {
		return std::visit([](const auto &desc) -> vk::DescriptorSet {
			return desc;
		}, descriptor);
	}
	
	void bind(const std::span <vk::WriteDescriptorSet, bindings> &writes)
	requires is_resource_group_v <Reference> {
		using T = Reference::struct_type;

		auto bind_one = [&] <size_t I> () {
			using Resource = T::fields::template get <I>;

			set_descriptor_write <Resource, I> (
				resource.template get <I> (),
				descriptor_handle(),
				writes[I],
				info_unions[I]
			);
		};

		constexpr_for(Is, bindings,
			(bind_one.template operator() <Is> (), ...)
		);
	}
	
	void bind(const std::span <vk::WriteDescriptorSet, bindings> &writes)
	requires (not is_resource_group_v <Reference>) {
		set_descriptor_write <Reference, 0> (
			resource,
			descriptor_handle(),
			writes[0],
			info_unions[0]
		);
	}
};

template <auto &ref>
DescriptorWrite(UnboundDescriptor <ref>, const ResourceTypeFor <ref> &)
	-> DescriptorWrite <ref>;

template <auto &ref>
DescriptorWrite(BoundDescriptor <ref>, const ResourceTypeFor <ref> &)
	-> DescriptorWrite <ref>;

template <auto &ref>
auto Device::new_descriptor(const DescriptorPool &dpool) const
	-> UnboundDescriptor <ref>
{
	auto key = _dsl_cache::Key(logical, &ref);
	auto dsl = _dsl_cache::map.at(key);
	auto dset = logical.allocateDescriptorSets(
		vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(dpool)
			.setSetLayouts(dsl)
	).front();
	return { dset };
}

template <auto &...refs>
[[nodiscard]] auto Device::update_descriptors(DescriptorWrite <refs> &&... dwpairs)
{
	static_assert(sizeof...(dwpairs) > 0);

	static constexpr size_t writes_count = (std::decay_t <decltype(dwpairs)> ::bindings + ...);

	std::array <vk::WriteDescriptorSet, writes_count> writes;

	auto progress = 0;
	auto bind = [&](auto &dwpair) {
		using pair_t = std::remove_reference_t <decltype(dwpair)>;
		constexpr size_t size = pair_t::bindings;
		auto span = std::span <vk::WriteDescriptorSet, size> {
			&writes[progress], size
		};
		dwpair.bind(span);
		progress += size;
	};

	(bind(dwpairs), ...);

	logical.updateDescriptorSets(writes, nullptr);

	if constexpr (sizeof...(dwpairs) == 1) {
		return BoundDescriptor <refs...[0]> {
			(dwpairs...[0]).descriptor_handle()
		};
	} else {
		return std::make_tuple(BoundDescriptor <refs> {
			dwpairs.descriptor_handle()
		}...);
	}
}

} // namespace rcgp
