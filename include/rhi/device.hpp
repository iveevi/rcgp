#pragma once

#include <span>
#include <vector>

#include "session.hpp"
#include "render_pass.hpp"
#include "queue.hpp"

namespace rcgp {

// Forward declarations
struct CommandPool;
struct DescriptorPool;
struct Frame;
struct CommandBuffer;
struct TimestampQueryPool;
struct TimestampQueryResult;

template <auto &ref, bool resolved>
struct DescriptorWritePair;

struct Device {
	vk::Device logical;
	vk::PhysicalDevice physical;
	vk::PhysicalDeviceMemoryProperties properties;
	vk::detail::DispatchLoaderDynamic loader;
	std::map <const char *, Queue> queues;

	auto find_memory_type(uint32_t filter, vk::MemoryPropertyFlags flags) const -> uint32_t;

	template <typename Extent>
	auto new_framebuffer(
		const vk::RenderPass &render_pass,
		const std::span <const vk::ImageView> &attachments,
		const Extent &extent,
		uint32_t layers = 1
	) const -> vk::Framebuffer {
		static_assert(std::same_as <Extent, vk::Extent2D>
			|| std::same_as <Extent, vk::Extent3D>);

		auto fb_info = vk::FramebufferCreateInfo();
		fb_info.renderPass = render_pass;
		fb_info.attachmentCount = attachments.size();
		fb_info.pAttachments = attachments.data();
		fb_info.width = extent.width;
		fb_info.height = extent.height;
		fb_info.layers = layers;

		return logical.createFramebuffer(fb_info);
	}

	// TODO: return neutral command buffers
	auto new_command_buffers(const CommandPool &cpool, size_t count) const -> std::vector <CommandBuffer>;
	auto new_descriptor_sets(const DescriptorPool &dpool, const vk::ArrayProxy <vk::DescriptorSetLayout> &dsls) const -> std::vector <vk::DescriptorSet>;

	auto new_shader_module(std::span <const uint32_t> spirv) const -> vk::ShaderModule {
		auto sm_info = vk::ShaderModuleCreateInfo();
		sm_info.codeSize = spirv.size_bytes();
		sm_info.pCode = spirv.data();
		return logical.createShaderModule(sm_info);
	}

	// TODO: un-template this...
	template <typename ... Ts>
	auto new_render_pass(const Attachments &attachments, Ts ... subpasses) const {
		auto rp_info = vk::RenderPassCreateInfo();
		rp_info.attachmentCount = attachments.descriptions.size();
		rp_info.pAttachments = attachments.descriptions.data();
		rp_info.subpassCount = sizeof...(subpasses);

		auto subpass_descritions = std::array <
			vk::SubpassDescription,
			sizeof...(subpasses)
		> { subpasses... };

		rp_info.subpassCount = subpass_descritions.size();
		rp_info.pSubpasses = subpass_descritions.data();

		return RenderPass <Ts...> (logical.createRenderPass(rp_info));
	}

	template <auto &...refs, bool ... rs>
	[[nodiscard]] auto update_descriptors(DescriptorWritePair <refs, rs> &&... dwpairs);

	void wait_for_frame(const Frame &frame, uint64_t timeout = UINT64_MAX) const;
	bool acquire_image_for_frame(Frame &frame, uint64_t timeout = UINT64_MAX) const;

	// Timestamp pool
	TimestampQueryPool new_timestamp_pool(vk::QueryResultFlags flags, size_t count) const;
	TimestampQueryResult get_timestamp_results(const TimestampQueryPool &tqpool) const;

	struct Options {
		std::vector <const char *> extensions;
		std::map <const char *, vk::QueueFlags> queues;
		// TODO: more general handling for device
		// features... custom enum?
		bool mesh_shaders = false;
		bool maintenance4 = false;
		bool scalar_block_layout = false;
		bool dynamic_rendering = false;
	};

	static Device from(
		const Session &session,
		vk::detail::DispatchLoaderDynamic &dld,
		const Options &info
	);
};

} // namespace rcgp
