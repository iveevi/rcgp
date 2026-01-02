#pragma once

#include <algorithm>
#include <type_traits>

#include "../../rhi/descriptor_pool.hpp"
#include "../collect_gvrs.hpp"
#include "../descriptor.hpp"
#include "../layout/all.hpp"
#include "../mirror_buffer.hpp"

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

// Compile-time stage flag lookup for a given resource reference within a sequence
template <auto &ref, typename Seq>
struct stage_flags_for_seq;

template <auto &ref>
struct stage_flags_for_seq <ref, sequence <>> {
	static constexpr vk::ShaderStageFlags value = {};
};

template <auto &ref, auto &other, ShaderStage ...Ss, typename ...Rest>
struct stage_flags_for_seq <
	ref,
	sequence <stage_wrapper <reference <other>, Ss...>, Rest...>
> {
	static constexpr vk::ShaderStageFlags value =
		std::same_as <reference <other>, reference <ref>>
			? stage_flags_of <Ss...> ()
			: stage_flags_for_seq <ref, sequence <Rest...>> ::value;
};

template <auto &ref, typename Seq>
constexpr vk::ShaderStageFlags stage_flags_for_v = stage_flags_for_seq <ref, Seq> ::value;

constexpr uint32_t align_up_u32(uint32_t value, uint32_t alignment)
{
	return (value + alignment - 1) / alignment * alignment;
}

template <typename SW>
struct push_constant_info;

template <auto &ref, ShaderStage ...Ss>
struct push_constant_info <stage_wrapper <reference <ref>, Ss...>> {
	using data_t = ResourceTypeOf <ref>;
	static constexpr uint32_t size = sizeof(data_t);
	static constexpr uint32_t alignment = std::max<uint32_t>(4u, alignof(data_t));
	static constexpr vk::ShaderStageFlags stage_flags = stage_flags_of <Ss...> ();
	static constexpr void *addr = (void *) &ref;

	static_assert(size % 4u == 0u, "push constant size must be a multiple of 4 bytes");
};

template <auto &ref, uint32_t Offset, typename Seq>
struct push_constant_offset_accum;

template <auto &ref, uint32_t Offset>
struct push_constant_offset_accum <ref, Offset, sequence <>> {
	static constexpr bool found = false;
	static constexpr uint32_t value = 0;
};

template <auto &ref, uint32_t Offset, typename Head, typename ... Rest>
struct push_constant_offset_accum <ref, Offset, sequence <Head, Rest...>> {
	static constexpr uint32_t aligned = align_up_u32(Offset, push_constant_info <Head> ::alignment);
	static constexpr bool matches = std::same_as <typename Head::type, reference <ref>>;
	static constexpr uint32_t next_offset = aligned + push_constant_info <Head> ::size;
	static constexpr bool found = matches || push_constant_offset_accum <ref, next_offset, sequence <Rest...>> ::found;
	static constexpr uint32_t value = matches
		? aligned
		: push_constant_offset_accum <ref, next_offset, sequence <Rest...>> ::value;
};

template <auto &ref, typename Seq>
constexpr bool push_constant_offset_found_v = push_constant_offset_accum <ref, 0, Seq> ::found;

template <auto &ref, typename Seq>
constexpr uint32_t push_constant_offset_for_v = push_constant_offset_accum <ref, 0, Seq> ::value;

// AttributeStreams := sequence <reference <Stream>...>
// GroupAllocation := sequence <reference <GRV>...>
// TODO: Sets should be inferred from GroupAllocation::size... or something
template <Topology T, typename AttributeStreams, typename GroupAllocation, typename GlobalResources, size_t Sets>
struct AnnotatedRasterizationPipeline {
	vk::Device device;
	vk::Pipeline handle;
	vk::PipelineLayout layout;
	std::array <vk::DescriptorSetLayout, Sets> dsls;

	using global_resources = GlobalResources;

	template <auto &ref>
	auto new_descriptor(const DescriptorPool &dpool) const {
		constexpr auto set = group_allocation_set_for <ref> (GroupAllocation());
		auto dset = device.allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo()
				.setDescriptorPool(dpool)
				.setSetLayouts(dsls[set])
		).front();
		// TODO: pass set as well...
		return DescriptorOf <ref, false> (dset);
	}
};
