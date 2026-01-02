#pragma once

#include <functional>

#include "pipeline/rasterization.hpp"
#include "vertex_buffer_of.hpp"

// Auxiliary command buffer state as dependency information
// TODO: concept for command state

struct CommandsTraceAux {
	vk::PipelineBindPoint bind_point;
	vk::PipelineLayout layout;
	// TODO: also track pipeline stage for syncing
};

using command_operator = std::function <void (const vk::CommandBuffer &, CommandsTraceAux &)>;

// TODO: profile/benchmark performance of commands
template <typename ... Effects>
struct Commands : std::vector <command_operator> {
	using std::vector <command_operator> ::vector;

	auto &operator()(const vk::CommandBuffer &cmd) const {
		CommandsTraceAux aux;

		cmd.reset();
		cmd.begin(vk::CommandBufferBeginInfo());

		for (auto &op : *this)
			op(cmd, aux);

		cmd.end();

		return cmd;
	}
};

template <typename ... EAs, typename ... EBs>
auto cmdcat(const Commands <EAs...> &x, const Commands <EBs...> &y)
{
	// TODO: simplify EAs + EBs
	auto cmd = Commands <EAs..., EBs...> {};
	cmd.append_range(x);
	cmd.append_range(y);
	return cmd;
}

template <typename First, typename Second, typename ...Rest>
auto cmdcat(const First &first, const Second &second, const Rest &... rest)
{
	static_assert(sizeof...(Rest) >= 0, "seq requires at least two arguments");
	auto combined = cmdcat(first, second);
	if constexpr (sizeof...(Rest) == 0)
		return combined;
	else
		return cmdcat(combined, rest...);
}

// Pipe operator: syntactic sugar for seq
template <typename ... EAs, typename ... EBs>
auto operator|(const Commands <EAs...> &lhs, const Commands <EBs...> &rhs)
{
	return cmdcat(lhs, rhs);
}

template <typename ... EAs>
auto operator|(const Commands <EAs...> &x, const std::nullptr_t &)
{
	return x;
}

template <typename ... EAs>
auto operator|(const std::nullptr_t &, const Commands <EAs...> &x)
{
	return x;
}

inline auto begin_render_pass(const vk::RenderPass &render_pass,
		       const vk::Framebuffer &framebuffer,
		       const vk::Rect2D &render_area,
		       const std::span <vk::ClearValue> &clear_values)
{
	auto binder = [=](const vk::CommandBuffer &cmd, CommandsTraceAux &) {
		auto rp_begin = vk::RenderPassBeginInfo()
			.setRenderPass(render_pass)
			.setFramebuffer(framebuffer)
			.setRenderArea(render_area)
			.setClearValues(clear_values);

		cmd.beginRenderPass(rp_begin, vk::SubpassContents::eInline);
	};

	return Commands <> { binder };
}

template <Topology T, typename AttributeStreams, typename GroupAllocation, typename GlobalResources, size_t Sets>
auto bind_pipeline(const AnnotatedRasterizationPipeline <T, AttributeStreams, GroupAllocation, GlobalResources, Sets> &pipeline)
{
	// TODO: write bind point and layout in an interm state (CommandBufferAux &)?
	auto binder = [=](const vk::CommandBuffer &cmd, CommandsTraceAux &aux) {
		aux.bind_point = vk::PipelineBindPoint::eGraphics;
		aux.layout = pipeline.layout;
		cmd.bindPipeline(aux.bind_point, pipeline.handle);
	};

	return Commands <> { binder };
}

template <auto &... refs>
auto bind_descriptors(const DescriptorOf <refs, true> &... descriptors)
{
	auto binder = [=](const vk::CommandBuffer &cmd, CommandsTraceAux &aux) {
		// TODO: store set index in the descriptor as well; populate when allocated

		// NOTE: for now assuming its in order and all at once
		
   		size_t idx = 0;
		(cmd.bindDescriptorSets(
			aux.bind_point,
			aux.layout,
			idx++, { descriptors }, {}
		), ...);
	};

	return Commands <> { binder };
}

// TODO: should be PushConstantOf <rsrc>
template <auto &ref, Topology T, typename AttributeStreams, typename GroupAllocation, typename GlobalResources, size_t Sets>
auto push_constants(const AnnotatedRasterizationPipeline <T, AttributeStreams, GroupAllocation, GlobalResources, Sets> &,
		    const ResourceTypeOf <ref> &constants)
{
	static_assert(is_push_constant_v <reference_base_t <ref>>);

	constexpr auto stage_flags = stage_flags_for_v <ref, GlobalResources>;
	static_assert(stage_flags != vk::ShaderStageFlags(), "push constant not used by any stage");
	static_assert(push_constant_offset_found_v <ref, GlobalResources>,
		"push constant not found in pipeline layout");
	static_assert(sizeof(ResourceTypeOf <ref>) % 4u == 0u,
		"push constant size must be a multiple of 4 bytes");

	auto binder = [stage_flags, constants](const vk::CommandBuffer &cmd, CommandsTraceAux &aux) {
		constexpr auto offset = push_constant_offset_for_v <ref, GlobalResources>;
		cmd.pushConstants <ResourceTypeOf <ref>> (aux.layout, stage_flags, offset, constants);
	};

	return Commands <> { binder };
}

inline auto unbind_pipeline()
{
	// Vulkan has no explicit unbind; this is a logical state transition helper.
	auto binder = [](const vk::CommandBuffer &, CommandsTraceAux &) {};

	return Commands <> { binder };
}

template <auto &... refs>
auto bind_vertex_buffers(const VertexBufferOf <refs> &... buffers)
{
	auto binder = [=](const vk::CommandBuffer &cmd, CommandsTraceAux &) {
		std::array <vk::DeviceSize, sizeof...(refs)> offsets {};
		cmd.bindVertexBuffers(0, { buffers.handle... }, offsets);
	};

	return Commands <> { binder };
}

// and add a state tag that encodes this... ProvidedIndexBuffer <T>
template <Topology T, typename I>
inline auto bind_index_buffer(const IndexBuffer <T, I> &ibuffer)
{
	// TODO: templates for inferring the index type
	auto binder = [=](const vk::CommandBuffer &cmd, CommandsTraceAux &) {
		cmd.bindIndexBuffer(ibuffer.handle, 0, vk::IndexType::eUint32);
	};

	return Commands <> { binder };
}

inline auto draw_indexed(uint32_t count)
{
	// TODO: should return an aux state which adds back the dependencies
	auto binder = [=](const vk::CommandBuffer &cmd, CommandsTraceAux &) {
		cmd.drawIndexed(count, 1, 0, 0, 0);
	};

	return Commands <> { binder };
}

inline auto end_render_pass()
{
	auto binder = [](const vk::CommandBuffer &cmd, CommandsTraceAux &) {
		cmd.endRenderPass();
	};

	return Commands <> { binder };
}
