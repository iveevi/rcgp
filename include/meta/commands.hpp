#pragma once

#include <functional>

#include "pipeline/rasterization.hpp"
#include "vertex_buffer_of.hpp"

// Primary command buffer state
// ----------------------------
//  * Any
//  * Neutral
//  * RenderPass
//  * RasterizationPipeline
enum class CommandFlag {
	aAny,
	eNeutral,
	eRenderPass,
	eRasterizationPipeline,
};
// TODO: decay rules:
// (Rasterization/MeshShading)Pipeline -> RenderPass
// (RayTracing/Compute)Pipeline -> Neutral
// RenderPass -> Neutral

// Auxiliary command buffer state as dependency information
// TODO: flags for phase?
template <auto &ref>
struct PendingAttributeStream {};

template <auto &ref>
struct PendingGlobalResource {};

// TODO: sync stuff goes here
// NOTE: in general there are various aux infos
// but they have an algebra

// NOTE: RPending and RProvided cancel each other out
// if they ref the same thing...

// TODO: concept for auxiliary states

// Ts are all auxiliary states
// TODO: some way to calculate the minimal state flags for barriers...
// at runtime (when flushing commands), would need to track the dependencies correctly...
template <CommandFlag S, typename ... Ts>
struct CommandState {
	static_assert(!((S == CommandFlag::eNeutral) && (sizeof...(Ts) > 0)),
		 "neutral states should have no other state");

	// TODO: append aux <T> -> <Ts..., T>
};

using NeutralCommandState = CommandState <CommandFlag::eNeutral>;

// TODO: concept for command state

struct CommandsTraceAux {
	vk::PipelineBindPoint bind_point;
	vk::PipelineLayout layout;
	// TODO: also track pipeline stage for syncing
};

using command_operator = std::function <void (const vk::CommandBuffer &, CommandsTraceAux &)>;

// TODO: profile/benchmark performance of commands
// X := CommandState <X, ...>
// Y := CommandState <Y, ...>
template <typename X, typename Y>
struct Commands : std::vector <command_operator> {
	using std::vector <command_operator> ::vector;

	auto &operator()(const vk::CommandBuffer &cmd) const {
		static_assert(std::same_as <X, NeutralCommandState>
			&& std::same_as <Y, NeutralCommandState>);

		CommandsTraceAux aux;

		cmd.reset();
		cmd.begin(vk::CommandBufferBeginInfo());

		for (auto &op : *this)
			op(cmd, aux);

		cmd.end();

		return cmd;
	}
};

using NeutralCommands = Commands <NeutralCommandState, NeutralCommandState>;

template <typename A, typename B, typename C, typename D>
auto seq(const Commands <A, B> &x, const Commands <C, D> &y)
{
	// TODO: general fusion operator <A, B, C, D> -> Commands <F::in, F::out>
	// TODO: is_compatible(B::flag, C::flag)
	// TODO: static assertions... fallback to agnostic if all else fails
	auto cmd = Commands <A, D> {};
	cmd.append_range(x);
	cmd.append_range(y);
	return cmd;
}

template <typename First, typename Second, typename ...Rest>
auto seq(const First &first, const Second &second, const Rest &... rest)
{
	static_assert(sizeof...(Rest) >= 0, "seq requires at least two arguments");
	auto combined = seq(first, second);
	if constexpr (sizeof...(Rest) == 0)
		return combined;
	else
		return seq(combined, rest...);
}

// Pipe operator: syntactic sugar for seq
template <typename A, typename B, typename C, typename D>
auto operator|(const Commands <A, B> &lhs, const Commands <C, D> &rhs)
{
	return seq(lhs, rhs);
}

template <typename A, typename B>
auto operator|(const Commands <A, B> &x, const std::nullptr_t &)
{
	return x;
}

template <typename A, typename B>
auto operator|(const std::nullptr_t &, const Commands <A, B> &x)
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

	return Commands <
		NeutralCommandState,
		CommandState <CommandFlag::eRenderPass>
	> { binder };
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

	return Commands <
		CommandState <CommandFlag::eRenderPass>,
		// TODO: put resource dependencies for all
		// streams and global resources
		CommandState <CommandFlag::eRasterizationPipeline>
	> { binder };
}

// TODO: should have some additional metadata for each descriptor
// which unambiguously identifies the pipeline layout... since
// descriptor set and stage usage may differ
//
// TODO: since this ordering is resolved statically, its probably sufficient
// to generate a unique type index for the pipeline, and then transfer
// that as an additional label to the descriptors;
// then everything can be resolved with the pendingX by attaching
// the ids to the pending states...
// use friend injection to register the pipeline's type under that generated index
//
// TODO: generally this type id + friend injection techique could be used
// to obfuscate/hide the actual type information from users (and compiler)
// to relieve some of the burden of looking at long types...
template <auto &... refs>
auto bind_descriptors(const DescriptorOf <refs, true> &... descriptors)
{
	// TODO: take resolved as template parameters, static assert if not all resolved

	// TODO: pass offset of first set as well
	auto binder = [=](const vk::CommandBuffer &cmd, CommandsTraceAux &aux) {
		// TODO: find the set index of each from the pipeline
		// (by looking its type up from the index)

		// NOTE: for now assuming its in order and all at once
		
   		size_t idx = 0;
		(cmd.bindDescriptorSets(
			aux.bind_point,
			aux.layout,
			idx++, { descriptors }, {}
		), ...);
	};

	// TODO: fetch the pipeline type associated to the descriptor pipeline id
	return Commands <
		CommandState <CommandFlag::eRasterizationPipeline>,
		CommandState <CommandFlag::eRasterizationPipeline>
	> { binder };
}

template <auto &ref, Topology T, typename AttributeStreams, typename GroupAllocation, typename GlobalResources, size_t Sets>
auto push_constants(const AnnotatedRasterizationPipeline <T, AttributeStreams, GroupAllocation, GlobalResources, Sets> &,
		    const ResourceMirrorOf <ref> &constants)
{
	static_assert(is_push_constant_v <reference_base_t <ref>>);

	constexpr auto stage_flags = stage_flags_for_v <ref, GlobalResources>;
	static_assert(stage_flags != vk::ShaderStageFlags(), "push constant not used by any stage");

	auto binder = [&, stage_flags](const vk::CommandBuffer &cmd, CommandsTraceAux &aux) {
		cmd.pushConstants(aux.layout, stage_flags, 0, sizeof(constants), &constants);
	};

	return Commands <
		CommandState <CommandFlag::eRasterizationPipeline>,
		CommandState <CommandFlag::eRasterizationPipeline>
	> { binder };
}

inline auto unbind_pipeline()
{
	// Vulkan has no explicit unbind; this is a logical state transition helper.
	auto binder = [](const vk::CommandBuffer &, CommandsTraceAux &) {};

	return Commands <
		CommandState <CommandFlag::eRasterizationPipeline>,
		CommandState <CommandFlag::eRenderPass>
	> { binder };
}

template <auto &... refs>
auto bind_vertex_buffers(const VertexBufferOf <refs> &... buffers)
{
	auto binder = [=](const vk::CommandBuffer &cmd, CommandsTraceAux &) {
		std::array <vk::DeviceSize, sizeof...(refs)> offsets {};
		cmd.bindVertexBuffers(0, { buffers.handle... }, offsets);
	};

	return Commands <
		CommandState <CommandFlag::eRasterizationPipeline>,
		CommandState <CommandFlag::eRasterizationPipeline>
	> { binder };
}

// and add a state tag that encodes this... ProvidedIndexBuffer <T>
template <Topology T, typename I>
inline auto bind_index_buffer(const IndexBuffer <T, I> &ibuffer)
{
	// TODO: templates for inferring the index type
	auto binder = [=](const vk::CommandBuffer &cmd, CommandsTraceAux &) {
		cmd.bindIndexBuffer(ibuffer.handle, 0, vk::IndexType::eUint32);
	};

	return Commands <
		CommandState <CommandFlag::eRasterizationPipeline>,
		CommandState <CommandFlag::eRasterizationPipeline>
	> { binder };
}

inline auto draw_indexed(uint32_t count)
{
	// TODO: should return an aux state which adds back the dependencies
	auto binder = [=](const vk::CommandBuffer &cmd, CommandsTraceAux &) {
		cmd.drawIndexed(count, 1, 0, 0, 0);
	};

	return Commands <
		CommandState <CommandFlag::eRasterizationPipeline>,
		CommandState <CommandFlag::eRasterizationPipeline>
	> { binder };
}

inline auto end_render_pass()
{
	auto binder = [](const vk::CommandBuffer &cmd, CommandsTraceAux &) {
		cmd.endRenderPass();
	};

	return Commands <
		CommandState <CommandFlag::eRenderPass>,
		NeutralCommandState
	> { binder };
}
