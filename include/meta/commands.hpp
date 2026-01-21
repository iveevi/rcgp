#pragma once

#include <array>
#include <functional>

#include "../rhi/command_buffer.hpp"
#include "../rhi/timestamp_pool.hpp"
#include "../util/cti.hpp"
#include "../util/runtime_type_registry.hpp"
#include "../util/timer.hpp"
#include "barrier.hpp"
#include "pipeline/compute.hpp"
#include "pipeline/mesh_shading.hpp"
#include "pipeline/rasterization.hpp"
#include "static_string.hpp"

struct PipelineMappings {
	vk::PipelineLayout layout;
	vk::PipelineBindPoint bind_point;
	std::map <void *, size_t> pc_offsets;
	std::map <void *, vk::ShaderStageFlags> pc_stages;
	std::map <void *, size_t> vb_offsets;

	static inline std::map <size_t, PipelineMappings> cache;
};

template <auto &... refs>
void write_vb_offsets(PipelineMappings &dst, Tlist <reference <refs>...>)
{
	size_t counter = 0;
	(dst.vb_offsets.emplace(&refs, counter++), ...);
}

template <typename ... Wrappers>
void write_pb_infos(PipelineMappings &dst, Tlist <Wrappers...>)
{
	using GRCs = Tlist <Wrappers...>;
	auto write = [&] <auto &ref> () {
		using Resource = reference_base_t <ref>;
		
		if constexpr (is_push_constant_v <Resource>) {
			auto flags = stage_flags_for_v <ref, GRCs>;
			constexpr auto offset = push_constant_offset_for <ref> (GRCs());
			dst.pc_stages.emplace(&ref, flags);
			dst.pc_offsets.emplace(&ref, offset);
		}
	};

	(write.template operator() <Wrappers::reference::handle> (), ...);
}

template <Topology T, typename AS, typename GAMAP, typename GRCs>
auto pipeline_mappings(const RasterizationPipeline <T, AS, GAMAP, GRCs> &pipeline)
{
	PipelineMappings result;
	result.layout = pipeline.layout;
	result.bind_point = vk::PipelineBindPoint::eGraphics;
	write_vb_offsets(result, AS());
	write_pb_infos(result, GRCs());
	return result;
}

template <typename GAMAP, typename GRCs>
auto pipeline_mappings(const ComputePipeline <GAMAP, GRCs> &pipeline)
{
	PipelineMappings result;
	result.layout = pipeline.layout;
	result.bind_point = vk::PipelineBindPoint::eCompute;
	write_pb_infos(result, GRCs());
	return result;
}

template <typename GAMAP, typename GRCs>
auto pipeline_mappings(const MeshShadingPipeline <GAMAP, GRCs> &pipeline)
{
	PipelineMappings result;
	result.layout = pipeline.layout;
	result.bind_point = vk::PipelineBindPoint::eGraphics;
	write_pb_infos(result, GRCs());
	return result;
}

// TODO: keep in mind of variant based command dispatch;
// downside is that custom/manual commands wouldn't be possible
struct SerializationContext {
	size_t pplid;
};

// Atomic command effects
// TODO: test resource group barriers/syncs with matrix mult example
// (SAXPY -> s * (A * X) + y should have 2 barriers; and test with sync validation)

// TODO: effect to confirm we are in a pipeline?
// or are residual resource bindings not allowed...
// dep <r> + res <r> -> sat <r>?
// but res <r> without anything should be X

// We have a dependency on ref
template <auto &ref>
struct Dependency {};

// We *might have a dependency for an index buffer for topology T
template <Topology T>
struct DependencyIndicatorForIndexBuffer {};

// We *have a dependency for an index buffer as needed (unknown topology)
struct DependencyEnforcerForIndexBuffer {};

// We *have a dependency for an index buffer for topology T
template <Topology T>
struct DependencyForIndexBuffer {};

// We are granted a handle for ref
template <auto &ref>
struct Resolvant {};

// We are granted a handle for an index buffer for topology T
template <Topology T>
struct ResolvantForIndexBuffer {};

// We need all previous dependencies to be resolved
struct DependencySentinel {};

// We have synchronized ref
template <auto &ref, typename Phase>
struct Sync {};

// TODO: algebra: communitivity...
// Ibuffer enforcer can commute backwards until it hits
// the indicator, after which its replaced by a real dependency
// failure occurs if there is no indicator
// dependency <r> + resolvant <r> = resolvant <r> (they dont cancel)

// Dependency sequences for annotated pipelines
// TODO: split headers...
template <auto &... refs>
consteval auto dependency_sequence_for_streams(Tlist <reference <refs>...>)
{
	return Tlist <Dependency <refs>...> {};
}

template <typename ... Wrappers>
consteval auto dependency_sequence_for_grcs(Tlist <Wrappers...>)
{
	return Tlist <Dependency <Wrappers::reference::handle>...> {};
}

template <Topology T, typename AS, typename GAMAP, typename GRCs>
consteval auto dependency_sequence(const RasterizationPipeline <T, AS, GAMAP, GRCs> &pipeline)
{
	auto ib = Tlist <DependencyIndicatorForIndexBuffer <T>> {};
	auto as = dependency_sequence_for_streams(AS());
	auto grcs = dependency_sequence_for_grcs(GRCs());
	return tlist_concat(ib, as, grcs);
}

template <typename GAMAP, typename GRCs>
consteval auto dependency_sequence(const ComputePipeline <GAMAP, GRCs> &pipeline)
{
	auto grcs = dependency_sequence_for_grcs(GRCs());
	return grcs;
}

template <typename GAMAP, typename GRCs>
consteval auto dependency_sequence(const MeshShadingPipeline <GAMAP, GRCs> &pipeline)
{
	auto grcs = dependency_sequence_for_grcs(GRCs());
	return grcs;
}

// TODO: more efficient std function representation?
// TODO: need to compare with slang version...
// ideally finish up specular lighting...
using command_operator = std::function <void (const CommandBuffer &, SerializationContext &)>;

template <typename ... Effects>
struct Commands : std::vector <command_operator> {
	using std::vector <command_operator> ::vector;

	void serialize(const CommandBuffer &cmd, SerializationContext &sctx) const {
		for (auto &op : *this)
			op(cmd, sctx);
	}

	// TODO: must be in a satisfied state...
	auto &operator()(CommandBuffer &cmd) const {
		TSCOPE("commands serialization");
		TNOTE("{} commands to trace", size());

		SerializationContext aux;

		cmd.reset();
		cmd.begin(vk::CommandBufferBeginInfo());

		serialize(cmd, aux);

		cmd.end();

		return cmd;
	}

	template <typename ... Es>
	friend auto operator|(const Commands &a, const Commands <Es...> &b) {
		// TODO: normalize <...>
		auto cmd = Commands <Effects..., Es...> {};
		cmd.append_range(a);
		cmd.append_range(b);
		return cmd;
	}

	friend auto operator|(const std::nullptr_t &, const Commands &x) {
		return x;
	}

	// TODO: allow |= if the result is the same
};

TYPE_TRAIT(is_commands);
	template <typename ... Effects>
	TYPE_TRAIT_INCLUDES(is_commands, Commands <Effects...>);

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
	constexpr auto dependencies = dependency_sequence(pipeline);

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

template <Topology T, typename I>
inline auto bind_index_buffer(const IndexBuffer <T, I> &ibuffer)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		if constexpr (std::same_as <I, uint32_t>)
			cmd.bindIndexBuffer(ibuffer.handle, 0, vk::IndexType::eUint32);
		else if constexpr (std::same_as <I, uint16_t>)
			cmd.bindIndexBuffer(ibuffer.handle, 0, vk::IndexType::eUint16);
		else
			static_error("unsupported index buffer scalar type"_ss);
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
