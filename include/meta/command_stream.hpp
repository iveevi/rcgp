#pragma once

#include "../rhi/command_buffer.hpp"
#include "descriptor.hpp"
#include "mirror_buffer.hpp"
#include "pipeline_mappings.hpp"
#include "pipelines.hpp"

namespace rcgp {

template <typename ... Buffers>
requires (is_vertex_buffer_v <Buffers> && ...)
struct DrawParameters {
	std::tuple <Buffers...> vertex_buffers;

	DrawParameters(const Buffers &... buffers)
		: vertex_buffers(buffers...) {}
};

// TODO: topology as a selector?
template <typename IndexBuffer, typename ... Buffers>
requires (is_index_buffer_v <IndexBuffer> and (is_vertex_buffer_v <Buffers> && ...))
struct DrawIndexedParameters {
	IndexBuffer index_buffer;
	vk::IndexType index_type;
	std::tuple <Buffers...> vertex_buffers;

	DrawIndexedParameters(const IndexBuffer &ibuffer, const Buffers &... buffers)
		: index_buffer(ibuffer),
			index_type(vk::IndexType::eUint32),
			vertex_buffers(buffers...) {}
};

struct DrawDispatchSize {
	uint32_t vertices = 0;
	uint32_t instances = 1;
};

template <typename Wrapper>
using UnwrapDesc = BoundDescriptor <Wrapper::contract::handle>;

template <typename Wrapper>
using UnwrapConst = ResourceTypeFor <Wrapper::contract::handle>;

template <typename GRCs>
using DescTuple = decltype([] {
	using DescWrappers = descriptable_resources_t <GRCs>;
	using DescList = DescWrappers::template map <UnwrapDesc>;
	using DescTuple = DescList::template invoke <std::tuple>;
	return DescTuple();
} ());

template <typename GRCs>
using ConstTuple = decltype([] {
	using ConstWrappers = push_constant_resources_t <GRCs>;
	using ConstList = ConstWrappers::template map <UnwrapConst>;
	using ConstTuple = ConstList::template invoke <std::tuple>;
	return ConstTuple();
} ());

#define lambda_apply(tuple, elems, expr) \
	std::apply([&](auto &... elems) { (expr); }, tuple)

struct CommandStream : CommandBuffer {
	using CommandBuffer::CommandBuffer;

	template <auto &... refs>
	void _bind_descriptors(
		const vk::PipelineLayout &layout,
		const BoundDescriptor <refs> &... descriptors
	) const
	{
		(super::bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics, layout,
			descriptors.set, { descriptors.handle }, {}
		), ...);
	}
	
	template <typename GRCs, typename ... Constants>
	void _bind_constants(
		const vk::PipelineLayout &layout,
		const Constants &... constants
	) const
	{
		using Wrappers = push_constant_resources_t <GRCs>;
		constexpr_for(Is, Wrappers::size,
			([&] {
				using W = Wrappers::template get <Is>;
				constexpr auto &ref = W::contract::handle;
				super::pushConstants <ResourceTypeFor <ref>> (
					layout,
					stage_flags_for_v <ref, GRCs>,
					push_constant_offset_for <ref> (GRCs()),
					constants
				);
			} (), ...)
		);
	}

	template <typename ... Buffers>
	void _bind_vertex_buffers(const Buffers &... buffers) const
	{
		uint32_t index = 0;
		(super::bindVertexBuffers(index++, { buffers.handle }, { 0 }), ...);
	}

	// NOTE: even with descriptor buffer/heap the principle stays the same:
	// (pipeline, descriptor sets/buffer/heap, push constants,
	// pipeline specific args..., e.g. data + dispatch size)
	template <Topology T, auto &... streams, typename GAMAP, typename GRCs>
	auto draw(
		const RasterizationPipeline <T, Tlist <contract <streams>...>, GAMAP, GRCs> &pipeline,
		const DescTuple <GRCs> &descriptors,
		const ConstTuple <GRCs> &push_constants,
		const DrawParameters <ResourceTypeFor <streams>...> &parameters,
		const DrawDispatchSize &dispatch_size
	) const {
		// TODO: check with caches
		super::bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.handle);
		lambda_apply(descriptors, descs, _bind_descriptors(pipeline.layout, descs...));
		lambda_apply(push_constants, constants, _bind_constants <GRCs> (pipeline.layout, constants...));
		lambda_apply(parameters.vertex_buffers, buffers, _bind_vertex_buffers(buffers...));
		super::draw(dispatch_size.vertices, dispatch_size.instances, 0, 0);
	}

	template <Topology T, auto &... streams, typename GAMAP, typename GRCs, typename IndexBuffer>
	requires is_index_buffer_v <IndexBuffer>
	auto draw_indexed(
		const RasterizationPipeline <T, Tlist <contract <streams>...>, GAMAP, GRCs> &pipeline,
		const DescTuple <GRCs> &descriptors,
		const ConstTuple <GRCs> &push_constants,
		const DrawIndexedParameters <IndexBuffer, ResourceTypeFor <streams>...> &parameters,
		const DrawDispatchSize &dispatch_size
	) const {
		super::bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.handle);
		lambda_apply(descriptors, descs, _bind_descriptors(pipeline.layout, descs...));
		lambda_apply(push_constants, constants, _bind_constants <GRCs> (pipeline.layout, constants...));
		lambda_apply(parameters.vertex_buffers, buffers, _bind_vertex_buffers(buffers...));
		super::bindIndexBuffer(parameters.index_buffer.handle, 0, parameters.index_type);
		super::drawIndexed(dispatch_size.vertices, dispatch_size.instances, 0, 0, 0);
	}
};

} // namespace rcgp
