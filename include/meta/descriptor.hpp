#pragma once

#include "reference_introspection.hpp"

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

	std::tuple <vk::DescriptorBufferInfo> descriptor_infos;

	void bind(const std::span <vk::WriteDescriptorSet, bindings> &writes) {
		std::get <0> (descriptor_infos) = resource.descriptor_info();
		writes[0]
			.setDstSet(descriptor)
			.setDstBinding(0)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setBufferInfo(std::get <0> (descriptor_infos));
	}
};

template <auto &...refs, bool ... rs>
[[nodiscard]] auto Device::update_descriptors(DescriptorWritePair <refs, rs> ... dwpairs)
{
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

	return std::make_tuple(DescriptorOf <refs, true> (dwpairs.descriptor)...);
}
