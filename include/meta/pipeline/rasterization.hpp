#pragma once

#include "../../rhi/descriptor_pool.hpp"
#include "../descriptor.hpp"

// AttributeStreams := sequence <reference <Stream>...>
// GroupAllocation := sequence <reference <GRV>...>
// TODO: Sets should be inferred from GroupAllocation::size... or something
template <typename AttributeStreams, typename GroupAllocation, size_t Sets>
struct AnnotatedRasterizationPipeline {
	vk::Device device;
	vk::Pipeline handle;
	vk::PipelineLayout layout;
	std::array <vk::DescriptorSetLayout, Sets> dsls;

	template <auto &ref>
	auto new_descriptor(const DescriptorPool &dpool) const {
		constexpr auto set = group_allocation_set_for <ref> (GroupAllocation());
		auto dset = device.allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo()
				.setDescriptorPool(dpool)
				.setSetLayouts(dsls[set])
		).front();
		return DescriptorOf <ref, false> (dset);
	}
};
