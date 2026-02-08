#pragma once

#include "../rhi/device.hpp"
#include "../util/cti.hpp"
#include "resources.hpp"
#include "witnesses.hpp"
#include "static_string.hpp"

namespace rcgp {

template <auto &ref, bool resolved = true>
struct DescriptorFor {
	vk::DescriptorSet handle;
	size_t set;
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

	write.dstSet = set;
	write.dstBinding = I;
	write.descriptorCount = 1;

	auto dinfo = resource.descriptor_info();
	if constexpr (is_sampler_v <T>) {
		write.descriptorType = eCombinedImageSampler;
		write.pImageInfo = &(info.image = dinfo);
	} else if constexpr (is_storage_buffer_v <T>) {
		write.descriptorType = eStorageBuffer;
		write.pBufferInfo = &(info.buffer = dinfo);
	} else {
		static_error("unsupported resource type "_ss + $ss_type(T));
	}
}

// Descriptor write handler with temporary storage
template <auto &ref, bool resolved>
struct DescriptorWritePair {
	using Reference = reference_base_of <ref>;

	const DescriptorFor <ref, resolved> &descriptor;
	const ResourceTypeFor <ref> &resource;

	static constexpr size_t bindings = number_of_bindings <Reference>;
	std::array <DescriptorInfoUnion, bindings> info_unions;
	
	void bind(const std::span <vk::WriteDescriptorSet, bindings> &writes)
	requires is_resource_group_v <Reference> {
		using T = Reference::struct_type;

		auto bind_one = [&] <size_t I> () {
			using Resource = T::fields::template get <I>;

			set_descriptor_write_and_union <Resource, I> (
				resource.template get <I> (),
				descriptor.handle,
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
			descriptor.handle,
			writes[0],
			info_unions[0]
		);
	}
};

template <auto &...refs, bool ... rs>
[[nodiscard]] auto Device::update_descriptors(DescriptorWritePair <refs, rs> &&... dwpairs)
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
		return DescriptorFor <refs...[0], true> (
			(dwpairs...[0]).descriptor.handle,
			(dwpairs...[0]).descriptor.set
		);
	} else {
		return std::make_tuple(DescriptorFor <refs, true> (
			dwpairs.descriptor.handle,
			dwpairs.descriptor.set
		)...);
	}
}

} // namespace rcgp
