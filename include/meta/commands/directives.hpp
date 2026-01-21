#pragma once

#include <array>
#include <span>
#include <type_traits>

#include "../../rhi/timestamp_pool.hpp"
#include "../../util/runtime_type_registry.hpp"
#include "../barrier.hpp"
#include "../static_string.hpp"
#include "command_effects.hpp"
#include "commands.hpp"
#include "pipeline_mappings.hpp"

// TODO: later encode the number of attachments, and subpass progression
inline auto begin_render_pass(
	const vk::RenderPass &render_pass,
	const vk::Framebuffer &framebuffer,
	const vk::Rect2D &render_area,
	const std::span <vk::ClearValue> &clear_values
)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		auto rp_begin = vk::RenderPassBeginInfo()
			.setRenderPass(render_pass)
			.setFramebuffer(framebuffer)
			.setRenderArea(render_area)
			.setClearValues(clear_values);

		cmd.beginRenderPass(rp_begin, vk::SubpassContents::eInline);
	};

	return Commands <> { binder };
}

TYPE_TRAIT(is_pipeline);
	template <Topology T, typename AS, typename GAMAP, typename GRCs>
	TYPE_TRAIT_INCLUDES(is_pipeline, RasterizationPipeline <T, AS, GAMAP, GRCs>);

	template <typename GA, typename GR>
	TYPE_TRAIT_INCLUDES(is_pipeline, ComputePipeline <GA, GR>);

	template <typename GA, typename GR>
	TYPE_TRAIT_INCLUDES(is_pipeline, MeshShadingPipeline <GA, GR>);

template <typename Pipeline>
requires is_pipeline_v <Pipeline>
auto bind_pipeline(const Pipeline &pipeline)
{
	static const size_t id = RuntimeTypeRegistry::id <Pipeline> ();

	if (not PipelineMappings::cache.contains(id)) {
		auto mappings = pipeline_mappings(pipeline);
		PipelineMappings::cache.emplace(id, mappings);
	}

	auto &cid = PipelineMappings::cache.at(id);
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &sctx) {
		sctx.pplid = id;
		cmd.bindPipeline(cid.bind_point, pipeline.handle);
	};

	// TODO: load up the dependencies
	constexpr auto dependencies = command_effects(pipeline);

	using list = decltype(dependencies);
	using cmds = list::template invoke <Commands>;

	return cmds { binder };
}

// TODO: for assurance (compatibility) should also template with a pipeline int ID
template <auto &... refs>
auto bind_descriptors(const DescriptorFor <refs, true> &... descriptors)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &sctx) {
		auto &cid = PipelineMappings::cache.at(sctx.pplid);
		(cmd.bindDescriptorSets(
			cid.bind_point,
			cid.layout,
			descriptors.set, { descriptors.handle }, {}
		), ...);
	};

	return Commands <Resolvant <refs>...> { binder };
}

template <auto &... refs>
auto bind_push_constants(const ResourceTypeFor <refs> &... constants)
{
	static_assert((is_push_constant_v <reference_base_t <refs>> && ...));

	auto binder = [=](const CommandBuffer &cmd, SerializationContext &sctx) {
		auto &cid = PipelineMappings::cache.at(sctx.pplid);
		auto &pc_stages = cid.pc_stages;
		auto &pc_offsets = cid.pc_offsets;
		(cmd.pushConstants <ResourceTypeFor <refs>> (
			cid.layout,
			pc_stages.at(&refs),
			pc_offsets.at(&refs),
			constants
		), ...);
	};

	return Commands <Resolvant <refs>...> { binder };
}

template <auto &... refs>
auto bind_vertex_buffers(const ResourceTypeFor <refs> &... buffers)
{
	static_assert((is_attribute_stream_v <reference_base_t <refs>> && ...));

	auto binder = [...handles = buffers.handle](
		const CommandBuffer &cmd,
		SerializationContext &sctx
	) {
		auto &vb_offsets = PipelineMappings::cache.at(sctx.pplid).vb_offsets;

		// TODO: if they are adjacent then a single dispatch is enough...
		// we can cache that info here once per id...
		// auto offsets = std::array <vk::DeviceSize, sizeof...(refs)> (0);
		// auto buffers = std::array { handles... };

		(cmd.bindVertexBuffers(vb_offsets.at(&refs), { handles }, { 0 }), ...);
	};

	return Commands <Resolvant <refs>...> { binder };
}

template <Topology T, reflected Symbolic>
inline auto bind_index_buffer(const IndexMirrorBuffer <Symbolic, layouts::scalar> &ibuffer)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		if constexpr (std::is_same_v <Symbolic, array <vector <uint32_t, 3>>> ) {
			static_assert(T == Topology::eTriangleList, "unexpected index topology");
			cmd.bindIndexBuffer(ibuffer.handle, 0, vk::IndexType::eUint32);
		} else if constexpr (std::is_same_v <Symbolic, array <scalar <uint32_t>>> ) {
			static_assert(T == Topology::eTriangleFan, "unexpected index topology");
			cmd.bindIndexBuffer(ibuffer.handle, 0, vk::IndexType::eUint32);
		} else {
			static_error("unsupported index buffer type"_ss);
		}
	};

	return Commands <ResolvantForIndexBuffer <T>> { binder };
}

inline auto draw_indexed(uint32_t count)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.drawIndexed(count, 1, 0, 0, 0);
	};

	return Commands <
		DependencyEnforcerForIndexBuffer,
		DependencySentinel
	> { binder };
}

inline auto draw_mesh_tasks(uint32_t x, uint32_t y = 1, uint32_t z = 1)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.drawMeshTasks(x, y, z);
	};

	return Commands <DependencySentinel> { binder };
}

inline auto end_render_pass()
{
	auto binder = [](const CommandBuffer &cmd, SerializationContext &) {
		cmd.endRenderPass();
	};

	return Commands <> { binder };
}

inline auto reset_query_pool(const TimestampQueryPool &tqpool, uint32_t first, uint32_t count)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.resetQueryPool(tqpool.handle, first, count);
	};

	return Commands <> { binder };
}

inline auto write_timestamp(
	vk::PipelineStageFlagBits stage,
	const TimestampQueryPool &tqpool,
	uint32_t index
)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.writeTimestamp(stage, tqpool.handle, index);
	};

	return Commands <> { binder };
}

inline auto manual_commands(auto F)
{
	return Commands <> { F };
}

template <auto &... refs, typename ... SrcPhases, typename ... DstPhases>
inline auto barriers(const Barrier <refs, SrcPhases, DstPhases> &... barriers)
{
	static constexpr size_t barrier_count =
		(Barrier <refs, SrcPhases, DstPhases> ::count + ... + 0);

	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		std::array <vk::BufferMemoryBarrier2, barrier_count> buffer_barriers {};
		size_t idx = 0;
		(barriers.write_to(buffer_barriers, idx), ...);
		cmd.pipelineBarrier2(vk::DependencyInfo().setBufferMemoryBarriers(buffer_barriers));
	};

	return Commands <Sync <refs, DstPhases>...> { binder };
}

template <typename ... Es, auto &ref, typename SrcPhase, typename DstPhase>
inline auto operator|(const Commands <Es...> &cmds, const Barrier <ref, SrcPhase, DstPhase> &b)
{
	return cmds | barriers(b);
}

template <auto &ref, typename SrcPhase, typename DstPhase, typename ... Es>
inline auto operator|(const Barrier <ref, SrcPhase, DstPhase> &b, const Commands <Es...> &cmds)
{
	return barriers(b) | cmds;
}

template <auto &ref, typename SrcPhase, typename DstPhase>
inline auto operator|(const std::nullptr_t &, const Barrier <ref, SrcPhase, DstPhase> &b)
{
	return barriers(b);
}


inline auto dispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.dispatch(x, y, z);
	};

	return Commands <DependencySentinel> { binder };
}

template <typename T, typename F>
requires is_commands_v <std::invoke_result_t <F, T>>
inline auto foreach(const std::vector <T> &container, F &&ftn)
{
	using C = std::invoke_result_t <F, T>;

	auto binder = [=](const CommandBuffer &cmd, SerializationContext &sctx) {
		TSCOPE("foreach serialization");
		TNOTE("container size of {}", container.size());

		for (auto &&value : container)
			ftn(value).serialize(cmd, sctx);
	};

	return C { binder };
}
