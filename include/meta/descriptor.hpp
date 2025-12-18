#pragma once

#include "../rhi/device.hpp"
#include "reference_introspection.hpp"
#include "resources.hpp"

template <auto &ref, bool resolved = true>
struct DescriptorOf : vk::DescriptorSet {};

// Descriptor writes between descriptor of a resource and its resource mirror
template <auto &ref, bool resolved>
struct DescriptorWritePair {
	const DescriptorOf <ref, resolved> &descriptor;
	const ResourceMirrorOf <ref> &resource;

        // Number of write items required, can be
	// determined without the pipeline
        static constexpr size_t bindings = 1;

	static constexpr bool is_sampler = is_sampler_v <reference_base_t <ref>>;
	using info_t = std::conditional_t <is_sampler, vk::DescriptorImageInfo, vk::DescriptorBufferInfo>;

	std::tuple <info_t> descriptor_infos;

	void bind(const std::span <vk::WriteDescriptorSet, bindings> &writes) {
		if constexpr (is_sampler) {
			std::get <0> (descriptor_infos) = resource.descriptor_info();
			writes[0]
				.setDstSet(descriptor)
				.setDstBinding(0)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setImageInfo(std::get <0> (descriptor_infos));
		} else {
			std::get <0> (descriptor_infos) = resource.descriptor_info();
			writes[0]
				.setDstSet(descriptor)
				.setDstBinding(0)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setBufferInfo(std::get <0> (descriptor_infos));
		}
	}
};

template <auto &...refs, bool ... rs>
[[nodiscard]] auto Device::update_descriptors(DescriptorWritePair <refs, rs> ... dwpairs)
{
	static_assert(sizeof...(dwpairs) > 0);

	static constexpr size_t writes_count = (decltype(dwpairs)::bindings + ...);

	std::array <vk::WriteDescriptorSet, writes_count> writes;

	auto progress = 0;
	auto bind = [&](auto dwpair) {
		constexpr size_t size = decltype(dwpair)::bindings;
		auto span = std::span <vk::WriteDescriptorSet, size> {
			&writes[progress], size
		};
		dwpair.bind(span);
		progress += size;
	};

	(bind(dwpairs), ...);

	logical.updateDescriptorSets(writes, nullptr);

	if constexpr (sizeof...(dwpairs) == 1) {
		return DescriptorOf <refs...[0], true>
			((dwpairs...[0]).descriptor);
	} else {
		return std::make_tuple(DescriptorOf <refs, true> (dwpairs.descriptor)...);
	}
}
