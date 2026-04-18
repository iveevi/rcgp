#pragma once

#include <array>
#include <optional>
#include <vector>
#include <type_traits>

#include "../rhi/command_buffer.hpp"
#include "../rhi/image.hpp"
#include "../rhi/timestamp_pool.hpp"
#include "../util/runtime_type_registry.hpp"
#include "barrier.hpp"
#include "command_effects.hpp"
#include "commands.hpp"
#include "descriptor.hpp"
#include "pipeline_mappings.hpp"
#include "static_string.hpp"

namespace rcgp {

// TODO: later encode the number of attachments, and subpass progression
template <size_t N>
auto begin_render_pass(
	const vk::RenderPass &render_pass,
	const vk::Framebuffer &framebuffer,
	const vk::Rect2D &render_area,
	std::array <vk::ClearValue, N> clear_values
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

	return Commands <false> { binder };
}

inline auto begin_rendering(
	const vk::Rect2D &render_area,
	std::vector <vk::RenderingAttachmentInfo> color_attachments,
	std::optional <vk::RenderingAttachmentInfo> depth_attachment = std::nullopt
)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		auto rendering = vk::RenderingInfo()
			.setRenderArea(render_area)
			.setLayerCount(1)
			.setColorAttachments(color_attachments);

		if (depth_attachment.has_value())
			rendering.setPDepthAttachment(&depth_attachment.value());

		cmd.beginRendering(rendering);
	};

	return Commands <false> { binder };
}

template <typename Pipeline>
auto bind_pipeline(const Pipeline &pipeline)
{
	constexpr bool is_pipeline = is_pipeline_v <Pipeline>;
	static_assert(is_pipeline, "bind_pipeline: arg@0 is not a pipeline type");

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

	constexpr auto dependencies = command_effects(pipeline);

	using list = decltype(dependencies);
	using cmds = commands_from_t <false, list>;

	return cmds { binder };
}

// TODO: for assurance (compatibility) should also template with a pipeline int ID
template <auto &... refs>
auto bind_descriptors(const BoundDescriptor <refs> &... descriptors)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &sctx) {
		auto &cid = PipelineMappings::cache.at(sctx.pplid);
		(cmd.bindDescriptorSets(
			cid.bind_point,
			cid.layout,
			cid.gamap.at(&refs), { vk::DescriptorSet(descriptors) }, {}
		), ...);
	};

	return Commands <false, Resolvant <refs>...> { binder };
}

template <auto &... refs>
auto bind_push_constants(const ResourceTypeFor <refs> &... constants)
{
	static_assert((is_push_constant_v <reference_base_of <refs>> && ...));

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

	return Commands <false, Resolvant <refs>...> { binder };
}

template <auto &... refs>
auto bind_vertex_buffers(const ResourceTypeFor <refs> &... buffers)
{
	static_assert((is_attribute_stream_v <reference_base_of <refs>> && ...));

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

	return Commands <false, Resolvant <refs>...> { binder };
}

template <Topology T, typename Symbolic>
auto bind_index_buffer(const IndexMirrorBuffer <Symbolic, layouts::scalar> &ibuffer)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		if constexpr (std::is_same_v <Symbolic, array <vector <uint32_t, 3>>> ) {
			static_assert(T == Topology::eTriangleList, "unexpected index topology");
			cmd.bindIndexBuffer(ibuffer.handle, 0, vk::IndexType::eUint32);
		} else if constexpr (std::is_same_v <Symbolic, array <vector <uint32_t, 2>>> ) {
			static_assert(T == Topology::eLineList, "unexpected index topology");
			cmd.bindIndexBuffer(ibuffer.handle, 0, vk::IndexType::eUint32);
		} else if constexpr (std::is_same_v <Symbolic, array <scalar <uint32_t>>> ) {
			static_assert(T == Topology::eTriangleFan, "unexpected index topology");
			cmd.bindIndexBuffer(ibuffer.handle, 0, vk::IndexType::eUint32);
		} else {
			static_assert(false, "unsupported index buffer type"_ss);
		}
	};

	return Commands <false, ResolvantForIndexBuffer <T>> { binder };
}

inline auto draw_indexed(uint32_t count)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.drawIndexed(count, 1, 0, 0, 0);
	};

	return Commands <
		false,
		DependencyEnforcerForIndexBuffer,
		TerminalSentinel <PipelineKind::eRasterization>
	> { binder };
}

inline auto draw(uint32_t count, uint32_t instances = 1)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.draw(count, instances, 0, 0);
	};

	return Commands <false, TerminalSentinel <PipelineKind::eRasterization>> { binder };
}

inline auto draw_mesh_tasks(uint32_t x, uint32_t y = 1, uint32_t z = 1)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.draw_mesh_tasks(x, y, z);
	};

	return Commands <false, TerminalSentinel <PipelineKind::eMeshShading>> { binder };
}

inline auto end_render_pass()
{
	auto binder = [](const CommandBuffer &cmd, SerializationContext &) {
		cmd.endRenderPass();
	};

	return Commands <false> { binder };
}

inline auto end_rendering()
{
	auto binder = [](const CommandBuffer &cmd, SerializationContext &) {
		cmd.endRendering();
	};

	return Commands <false> { binder };
}

inline auto set_viewport(
	const vk::Viewport &viewport,
	uint32_t first_viewport = 0
)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.setViewport(first_viewport, std::array { viewport });
	};

	return Commands <false> { binder };
}

inline auto set_scissor(
	const vk::Rect2D &scissor,
	uint32_t first_scissor = 0
)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.setScissor(first_scissor, std::array { scissor });
	};

	return Commands <false> { binder };
}

inline auto set_viewport_scissor(
	vk::Extent2D extent,
	vk::Offset2D offset = vk::Offset2D(0, 0),
	float min_depth = 0.0f,
	float max_depth = 1.0f
)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		auto viewport = vk::Viewport()
			.setX(float(offset.x))
			.setY(float(offset.y))
			.setWidth(float(extent.width))
			.setHeight(float(extent.height))
			.setMinDepth(min_depth)
			.setMaxDepth(max_depth);

		auto scissor = vk::Rect2D()
			.setOffset(offset)
			.setExtent(extent);

		cmd.setViewport(0, std::array { viewport });
		cmd.setScissor(0, std::array { scissor });
	};

	return Commands <false> { binder };
}

inline auto reset_query_pool(const TimestampQueryPool &tqpool, uint32_t first, uint32_t count)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.resetQueryPool(tqpool.handle, first, count);
	};

	return Commands <false> { binder };
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

	return Commands <false> { binder };
}

inline auto transition(Image *image, vk::ImageLayout new_layout)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.transition(*image, new_layout);
	};

	return Commands <false> { binder };
}

inline auto transition(Image *image, vk::ImageLayout new_layout, const BarrierDesc &desc)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.transition(*image, new_layout, desc);
	};

	return Commands <false> { binder };
}

inline auto copy_image(const Image *const src, const Image *const dst)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.copy_image(*src, *dst);
	};

	return Commands <false> { binder };
}

inline auto blit_image(
    const Image *src,
    vk::ImageLayout src_layout,
    const Image *dst,
    vk::ImageLayout dst_layout,
    const vk::ImageBlit2 &region,
    vk::Filter filter = vk::Filter::eNearest
)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.blitImage2(vk::BlitImageInfo2()
			.setSrcImage(src->handle)
			.setSrcImageLayout(src_layout)
			.setDstImage(dst->handle)
			.setDstImageLayout(dst_layout)
			.setRegions(region)
			.setFilter(filter));
	};

	return Commands <false> { binder };
}

inline auto manual_commands(auto F)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		F(cmd);
	};

	return Commands <false> { binder };
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

	return Commands <false, BarrierEffect <refs, SrcPhases, DstPhases>...> { binder };
}

template <bool Live, typename ... Es, auto &ref, typename SrcPhase, typename DstPhase>
[[nodiscard]] auto operator|(const Commands <Live, Es...> &cmds, const Barrier <ref, SrcPhase, DstPhase> &b)
{
	return cmds | barriers(b);
}

template <auto &ref, typename SrcPhase, typename DstPhase, bool Live, typename ... Es>
[[nodiscard]] auto operator|(const Barrier <ref, SrcPhase, DstPhase> &b, const Commands <Live, Es...> &cmds)
{
	return barriers(b) | cmds;
}

template <auto &ref, typename SrcPhase, typename DstPhase>
[[nodiscard]] auto operator|(const std::nullptr_t &, const Barrier <ref, SrcPhase, DstPhase> &b)
{
	return barriers(b);
}


inline auto dispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.dispatch(x, y, z);
	};

	return Commands <false, TerminalSentinel <PipelineKind::eCompute>> { binder };
}

inline auto trace_rays(
    const vk::StridedDeviceAddressRegionKHR &raygen,
    const vk::StridedDeviceAddressRegionKHR &miss,
    const vk::StridedDeviceAddressRegionKHR &hit,
    const vk::StridedDeviceAddressRegionKHR &callable,
    uint32_t width, uint32_t height, uint32_t depth = 1
)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.traceRaysKHR(raygen, miss, hit, callable, width, height, depth, *cmd.loader);
	};

	return Commands <false, TerminalSentinel <PipelineKind::eRayTracing>> { binder };
}

template <typename T, typename F>
auto foreach(const std::vector <T> &container, F &&ftn)
{
	constexpr bool is_invocable = std::is_invocable_v <F, T>;
	static_assert(is_invocable, "foreach: arg@1 is not invocable with the container element type");
	using C = std::invoke_result_t <F, T>;
	constexpr bool returns_commands = is_commands_v <C>;
	static_assert(returns_commands, "foreach: arg@1 must return a Commands module");

	auto binder = [=](const CommandBuffer &cmd, SerializationContext &sctx) {
		for (auto &&value : container)
			for (auto &op : ftn(value))
				op(cmd, sctx);
	};

	return C { binder };
}

template <typename T, typename F>
auto branch(bool condition, T &&if_true, F &&if_false)
{
	constexpr bool t_invocable = std::is_invocable_v <T>;
	constexpr bool f_invocable = std::is_invocable_v <F>;
	static_assert(t_invocable, "branch: arg@1 must be invocable with no arguments");
	static_assert(f_invocable, "branch: arg@2 must be invocable with no arguments");

	using TC = std::invoke_result_t <T>;
	using FC = std::invoke_result_t <F>;

	constexpr bool t_is_commands = is_commands_v <TC>;
	constexpr bool f_is_commands = is_commands_v <FC>;
	static_assert(t_is_commands, "branch: arg@1 must return a Commands module");
	static_assert(f_is_commands, "branch: arg@2 must return a Commands module");

	constexpr bool arms_match = std::is_same_v <TC, FC>;
	static_assert(arms_match,
		"branch: both arms must produce identical Commands types; "
		"any divergence in effects (bindings, dispatches, barriers) is rejected");

	auto binder = [=](const CommandBuffer &cmd, SerializationContext &sctx) {
		if (condition) {
			for (auto &op : if_true())
				op(cmd, sctx);
		} else {
			for (auto &op : if_false())
				op(cmd, sctx);
		}
	};

	return TC { binder };
}

} // namespace rcgp
