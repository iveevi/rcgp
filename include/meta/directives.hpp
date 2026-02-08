#pragma once

#include <array>
#include <optional>
#include <vector>
#include <type_traits>

#include "../rhi/command_buffer.hpp"
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
	VkRenderPass render_pass,
	VkFramebuffer framebuffer,
	const VkRect2D &render_area,
	std::array <VkClearValue, N> clear_values
)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		VkRenderPassBeginInfo rp_begin {};
		rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		rp_begin.renderPass = render_pass;
		rp_begin.framebuffer = framebuffer;
		rp_begin.renderArea = render_area;
		rp_begin.clearValueCount = clear_values.size();
		rp_begin.pClearValues = clear_values.data();

		vkCmdBeginRenderPass(cmd.handle, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
	};

	return Commands <> { binder };
}

inline auto begin_rendering(
	const VkRect2D &render_area,
	std::vector <VkRenderingAttachmentInfo> color_attachments,
	std::optional <VkRenderingAttachmentInfo> depth_attachment = std::nullopt
)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		VkRenderingInfo rendering {};
		rendering.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		rendering.renderArea = render_area;
		rendering.layerCount = 1;
		rendering.colorAttachmentCount = uint32_t(color_attachments.size());
		rendering.pColorAttachments = color_attachments.data();

		if (depth_attachment.has_value())
			rendering.pDepthAttachment = &depth_attachment.value();

		vkCmdBeginRendering(cmd.handle, &rendering);
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
		vkCmdBindPipeline(cmd.handle, cid.bind_point, pipeline.handle);
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
		([
			&
		] {
			auto set = descriptors.handle;
			vkCmdBindDescriptorSets(
				cmd.handle,
				cid.bind_point,
				cid.layout,
				descriptors.set,
				1,
				&set,
				0,
				nullptr
			);
		} (), ...);
	};

	return Commands <Resolvant <refs>...> { binder };
}

template <auto &... refs>
auto bind_push_constants(const ResourceTypeFor <refs> &... constants)
{
	static_assert((is_push_constant_v <reference_base_of <refs>> && ...));

	auto binder = [=](const CommandBuffer &cmd, SerializationContext &sctx) {
		auto &cid = PipelineMappings::cache.at(sctx.pplid);
		auto &pc_stages = cid.pc_stages;
		auto &pc_offsets = cid.pc_offsets;
		(vkCmdPushConstants(
			cmd.handle,
			cid.layout,
			pc_stages.at(&refs),
			pc_offsets.at(&refs),
			sizeof(ResourceTypeFor <refs>),
			&constants
		), ...);
	};

	return Commands <Resolvant <refs>...> { binder };
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
		// auto offsets = std::array <VkDeviceSize, sizeof...(refs)> (0);
		// auto buffers = std::array { handles... };

		constexpr VkDeviceSize offset = 0;
		(vkCmdBindVertexBuffers(cmd.handle, vb_offsets.at(&refs), 1, &handles, &offset), ...);
	};

	return Commands <Resolvant <refs>...> { binder };
}

template <Topology T, typename Symbolic>
auto bind_index_buffer(const IndexMirrorBuffer <Symbolic, layouts::scalar> &ibuffer)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		if constexpr (std::is_same_v <Symbolic, array <vector <uint32_t, 3>>> ) {
			static_assert(T == Topology::eTriangleList, "unexpected index topology");
			vkCmdBindIndexBuffer(cmd.handle, ibuffer.handle, 0, VK_INDEX_TYPE_UINT32);
		} else if constexpr (std::is_same_v <Symbolic, array <scalar <uint32_t>>> ) {
			static_assert(T == Topology::eTriangleFan, "unexpected index topology");
			vkCmdBindIndexBuffer(cmd.handle, ibuffer.handle, 0, VK_INDEX_TYPE_UINT32);
		} else {
			static_error("unsupported index buffer type"_ss);
		}
	};

	return Commands <ResolvantForIndexBuffer <T>> { binder };
}

inline auto draw_indexed(uint32_t count)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		vkCmdDrawIndexed(cmd.handle, count, 1, 0, 0, 0);
	};

	return Commands <
		DependencyEnforcerForIndexBuffer,
		DependencySentinel
	> { binder };
}

inline auto draw_mesh_tasks(uint32_t x, uint32_t y = 1, uint32_t z = 1)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.draw_mesh_tasks(x, y, z);
	};

	return Commands <DependencySentinel> { binder };
}

inline auto end_render_pass()
{
	auto binder = [](const CommandBuffer &cmd, SerializationContext &) {
		vkCmdEndRenderPass(cmd.handle);
	};

	return Commands <> { binder };
}

inline auto end_rendering()
{
	auto binder = [](const CommandBuffer &cmd, SerializationContext &) {
		vkCmdEndRendering(cmd.handle);
	};

	return Commands <> { binder };
}

inline auto reset_query_pool(const TimestampQueryPool &tqpool, uint32_t first, uint32_t count)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		vkCmdResetQueryPool(cmd.handle, tqpool.handle, first, count);
	};

	return Commands <> { binder };
}

inline auto write_timestamp(
	VkPipelineStageFlagBits stage,
	const TimestampQueryPool &tqpool,
	uint32_t index
)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		vkCmdWriteTimestamp(cmd.handle, stage, tqpool.handle, index);
	};

	return Commands <> { binder };
}

inline auto transition_image_layout(Image *image, VkImageLayout new_layout)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.transition_image_layout(*image, new_layout);
	};

	return Commands <> { binder };
}

inline auto copy_image(const Image *const src, const Image *const dst)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		cmd.copy_image(*src, *dst);
	};

	return Commands <> { binder };
}

inline auto manual_commands(auto F)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		F(cmd);
	};

	return Commands <> { binder };
}

template <auto &... refs, typename ... SrcPhases, typename ... DstPhases>
auto barriers(const Barrier <refs, SrcPhases, DstPhases> &... barriers)
{
	static constexpr size_t barrier_count =
		(Barrier <refs, SrcPhases, DstPhases> ::count + ... + 0);

	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		std::array <VkBufferMemoryBarrier2, barrier_count> buffer_barriers {};
		size_t idx = 0;
		
		(barriers.write_to(buffer_barriers, idx), ...);
		
		VkDependencyInfo dep_info {};
		dep_info.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dep_info.bufferMemoryBarrierCount = barrier_count;
		dep_info.pBufferMemoryBarriers = buffer_barriers.data();

		vkCmdPipelineBarrier2(cmd.handle, &dep_info);
	};

	return Commands <BarrierEffect <refs, SrcPhases, DstPhases>...> { binder };
}

template <typename ... Es, auto &ref, typename SrcPhase, typename DstPhase>
auto operator|(const Commands <Es...> &cmds, const Barrier <ref, SrcPhase, DstPhase> &b)
{
	return cmds | barriers(b);
}

template <auto &ref, typename SrcPhase, typename DstPhase, typename ... Es>
auto operator|(const Barrier <ref, SrcPhase, DstPhase> &b, const Commands <Es...> &cmds)
{
	return barriers(b) | cmds;
}

template <auto &ref, typename SrcPhase, typename DstPhase>
auto operator|(const std::nullptr_t &, const Barrier <ref, SrcPhase, DstPhase> &b)
{
	return barriers(b);
}


inline auto dispatch(uint32_t x, uint32_t y = 1, uint32_t z = 1)
{
	auto binder = [=](const CommandBuffer &cmd, SerializationContext &) {
		vkCmdDispatch(cmd.handle, x, y, z);
	};

	return Commands <DependencySentinel> { binder };
}

template <typename T, typename F>
requires is_commands_v <std::invoke_result_t <F, T>>
auto foreach(const std::vector <T> &container, F &&ftn)
{
	using C = std::invoke_result_t <F, T>;

	auto binder = [=](const CommandBuffer &cmd, SerializationContext &sctx) {

		for (auto &&value : container)
			ftn(value).serialize(cmd, sctx);
	};

	return C { binder };
}

} // namespace rcgp
