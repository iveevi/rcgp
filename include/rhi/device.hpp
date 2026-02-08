#pragma once

#include <array>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <span>
#include <type_traits>
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
	VkDevice logical = VK_NULL_HANDLE;
	VkPhysicalDevice physical = VK_NULL_HANDLE;
	VkPhysicalDeviceMemoryProperties properties {};
	DispatchLoader loader;
	std::map <const char *, Queue> queues;

	auto find_memory_type(uint32_t filter, VkMemoryPropertyFlags flags) const -> uint32_t;

	template <typename Extent>
	auto new_framebuffer(
		VkRenderPass render_pass,
		const std::span <const VkImageView> &attachments,
		const Extent &extent,
		uint32_t layers = 1
	) const -> VkFramebuffer {
		static_assert(std::same_as <Extent, VkExtent2D>
			|| std::same_as <Extent, VkExtent3D>);

		VkFramebufferCreateInfo fb_info {};
		fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fb_info.renderPass = render_pass;
		fb_info.attachmentCount = attachments.size();
		fb_info.pAttachments = attachments.data();
		fb_info.width = extent.width;
		fb_info.height = extent.height;
		fb_info.layers = layers;

		auto framebuffer = VkFramebuffer(VK_NULL_HANDLE);
		auto result = vkCreateFramebuffer(logical, &fb_info, nullptr, &framebuffer);
		if (result != VK_SUCCESS)
			std::abort();
		return framebuffer;
	}

	// TODO: return neutral command buffers
	auto new_command_buffers(const CommandPool &cpool, size_t count) const -> std::vector <CommandBuffer>;
	auto new_descriptor_sets(const DescriptorPool &dpool, const std::span <const VkDescriptorSetLayout> &dsls) const -> std::vector <VkDescriptorSet>;

	auto new_shader_module(std::span <const uint32_t> spirv) const -> VkShaderModule {
		VkShaderModuleCreateInfo sm_info {};
		sm_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		sm_info.codeSize = spirv.size_bytes();
		sm_info.pCode = spirv.data();

		auto module = VkShaderModule(VK_NULL_HANDLE);
		auto result = vkCreateShaderModule(logical, &sm_info, nullptr, &module);
		if (result != VK_SUCCESS)
			std::abort();
		return module;
	}

	// TODO: un-template this...
	template <typename ... Ts>
	auto new_render_pass(const Attachments &attachments, Ts ... subpasses) const {
		auto subpass_descriptions = std::array <
			VkSubpassDescription,
			sizeof...(subpasses)
		> { subpasses... };

		VkRenderPassCreateInfo rp_info {};
		rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		rp_info.attachmentCount = attachments.descriptions.size();
		rp_info.pAttachments = attachments.descriptions.data();
		rp_info.subpassCount = subpass_descriptions.size();
		rp_info.pSubpasses = subpass_descriptions.data();

		auto render_pass = VkRenderPass(VK_NULL_HANDLE);
		auto result = vkCreateRenderPass(logical, &rp_info, nullptr, &render_pass);
		if (result != VK_SUCCESS)
			std::abort();

		return RenderPass <Ts...> (render_pass);
	}

	template <auto &...refs, bool ... rs>
	[[nodiscard]] auto update_descriptors(DescriptorWritePair <refs, rs> &&... dwpairs);

	void wait_for_frame(const Frame &frame, uint64_t timeout = UINT64_MAX) const;
	bool acquire_image_for_frame(Frame &frame, uint64_t timeout = UINT64_MAX) const;

	// Timestamp pool
	TimestampQueryPool new_timestamp_pool(VkQueryResultFlags flags, size_t count) const;
	TimestampQueryResult get_timestamp_results(const TimestampQueryPool &tqpool) const;

	struct Options {
		std::vector <const char *> extensions;
		std::map <const char *, VkQueueFlags> queues;
		// TODO: more general handling for device
		// features... custom enum?
		bool mesh_shaders = false;
		bool maintenance4 = false;
		bool scalar_block_layout = false;
		bool dynamic_rendering = false;
	};

	static Device from(
		const Session &session,
		DispatchLoader &dld,
		const Options &info
	);
};

} // namespace rcgp
