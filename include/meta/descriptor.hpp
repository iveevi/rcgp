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
union DescriptorInfoUnion {
	vk::DescriptorImageInfo image;
	vk::DescriptorBufferInfo buffer;
};

template <typename T, size_t I>
void set_descriptor_write_and_union(
	const auto &resource,
	const vk::DescriptorSet &set,
	vk::WriteDescriptorSet &write,
	DescriptorInfoUnion &info
)
{
	using enum vk::DescriptorType;

	write // ...
		.setDstSet(set)
		.setDstBinding(I);

	auto dinfo = resource.descriptor_info();
	if constexpr (is_sampler_v <T>) {
		write // ...
			.setDescriptorType(eCombinedImageSampler)
			.setImageInfo(info.image = dinfo);
	} else if constexpr (is_storage_buffer_v <T>) {
		write // ...
			.setDescriptorType(eStorageBuffer)
			.setBufferInfo(info.buffer = dinfo);
	} else if constexpr (is_uniform_buffer_v <T>) {
		write // ...
			.setDescriptorType(eUniformBuffer)
			.setBufferInfo(info.buffer = dinfo);
	} else {
		static_error("unsupported resource type "_ss + $ss_type(T));
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
	std::array <DescriptorInfoUnion, bindings> info_unions;

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

			set_descriptor_write_and_union <Resource, I> (
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
		set_descriptor_write_and_union <Reference, 0> (
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
