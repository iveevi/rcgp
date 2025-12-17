#pragma once

#include <type_traits>

#include "../../rhi/descriptor_pool.hpp"
#include "../descriptor.hpp"
#include "../layout/all.hpp"

enum class Topology {
	eTriangleList,
	eTriangleFan,
};

// Index buffer specialization
template <Topology T, typename I>
using topology_element_t = std::conditional_t <
	T == Topology::eTriangleList,
	vector <I, 3>,
	scalar <I>
>;

template <Topology T, typename I>
using index_buffer_base_t = IndexMirrorBuffer <
	array <topology_element_t <T, I>>,
	layouts::scalar
>;

template <Topology T, typename I>
struct IndexBuffer : index_buffer_base_t <T, I> {
	using base = index_buffer_base_t <T, I>;

	IndexBuffer() = default;
	IndexBuffer(const base &other) : base(other) {}

	auto &write(const base::value_type &data) const {
		base::write(data);
		return *this;
	}
	
	static IndexBuffer from(const Device &device,
			  	size_t max_elements,
			  	vk::MemoryPropertyFlags properties,
			  	vk::BufferUsageFlags extra_usage = vk::BufferUsageFlagBits(0)) {
		return base::from(device, max_elements, properties, extra_usage);
	}
};

// Translator for index buffers
template <Topology T, typename I>
struct resource_translator <IndexBuffer <T, I>> {
	using type = IndexBuffer <T, I>;
	using value_type = typename type::value_type;
	using element_type = typename type::element_type;
};

// AttributeStreams := sequence <reference <Stream>...>
// GroupAllocation := sequence <reference <GRV>...>
// TODO: Sets should be inferred from GroupAllocation::size... or something
template <Topology T, typename AttributeStreams, typename GroupAllocation, size_t Sets>
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
