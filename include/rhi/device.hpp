#pragma once

#include <map>
#include <span>
#include <string>
#include <vector>

#include "session.hpp"
#include "render_pass.hpp"
#include "queue.hpp"

namespace rcgp {

// Forward declarations
struct CommandBuffer;
struct CommandPool;
struct CommandStream;
struct DescriptorPool;
struct Frame;
struct TimestampQueryPool;
struct TimestampQueryResult;
struct Window;

enum class FrameAcquireStatus {
	Ok,
	Suboptimal,
	OutOfDate,
};

template <auto &ref>
struct UnboundDescriptor;

template <auto &ref>
struct DescriptorWrite;

vk::DebugUtilsObjectNameInfoEXT name_info(const vk::CommandBuffer &);

struct Device {
	vk::Device logical;
	vk::PhysicalDevice physical;
	vk::PhysicalDeviceMemoryProperties properties;
	vk::detail::DispatchLoaderDynamic loader;
	std::map <std::string, Queue> queues;

	void destroy() {
		logical.destroy();
	}

	void set_name(const auto &object, const std::string &name) {
		auto info = name_info(object)
			.setPObjectName(name.c_str());
		logical.setDebugUtilsObjectNameEXT(info, loader);
	}

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

		auto fb_info = vk::FramebufferCreateInfo()
			.setRenderPass(render_pass)
			.setAttachments(attachments)
			.setWidth(extent.width)
			.setHeight(extent.height)
			.setLayers(layers);

		return logical.createFramebuffer(fb_info);
	}

	auto new_command_buffers(const CommandPool &cpool, size_t count) const -> std::vector <CommandBuffer>;
	auto new_command_streams(const CommandPool &cpool, size_t count) const -> std::vector <CommandStream>;
	auto new_descriptor_sets(const DescriptorPool &dpool, const vk::ArrayProxy <vk::DescriptorSetLayout> &dsls) const -> std::vector <vk::DescriptorSet>;

	template <auto &ref>
	auto new_descriptor(const DescriptorPool &dpool) const -> UnboundDescriptor <ref>;

	auto new_shader_module(std::span <const uint32_t> spirv) const -> vk::ShaderModule {
		auto info = vk::ShaderModuleCreateInfo()
			.setCodeSize(spirv.size() * sizeof(uint32_t))
			.setPCode(spirv.data());

		return logical.createShaderModule(info);
	}

	auto new_render_pass(
		const Attachments &attachments,
		std::span <const Subpass> subpasses
	) const -> vk::RenderPass;

	template <auto &...refs>
	[[nodiscard]] auto update_descriptors(DescriptorWrite <refs> &&... dwpairs);

	void wait_for_frame(const Frame &frame, uint64_t timeout = UINT64_MAX) const;
	void reset_frame_fence(const Frame &frame) const;
	FrameAcquireStatus acquire_image_for_frame(Window &window, Frame &frame, uint64_t timeout = UINT64_MAX) const;
	bool rebuild_swapchain(Window &window) const;

	// Timestamp pool
	TimestampQueryPool new_timestamp_pool(vk::QueryResultFlags flags, size_t count) const;
	TimestampQueryResult get_timestamp_results(const TimestampQueryPool &tqpool) const;

	struct Options {
		std::vector <const char *> extensions;
		std::map <std::string, vk::QueueFlags> queues;
		void *pNext;
	};

	static Device from(
		const Session &session,
		vk::detail::DispatchLoaderDynamic &dld,
		const Options &info
	);
};

} // namespace rcgp
