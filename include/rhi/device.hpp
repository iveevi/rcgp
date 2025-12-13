#pragma once

#include <vector>

#include "session.hpp"
#include "render_pass.hpp"

// Forward declarations
struct CommandPool;
struct DescriptorPool;
struct Frame;

template <auto &ref, bool resolved>
struct DescriptorWritePair;

struct Device {
	vk::Device logical;
	vk::PhysicalDevice physical;
	vk::PhysicalDeviceMemoryProperties properties;
	vk::detail::DispatchLoaderDynamic loader;

	// Device construction
	struct Info {
		std::vector <const char *> extensions;
	};

	auto find_memory_type(uint32_t filter, vk::MemoryPropertyFlags flags) const -> uint32_t;

	auto new_framebuffer(
		const vk::RenderPass &render_pass,
		const std::span <const vk::ImageView> &attachments,
		const vk::Extent2D &extent,
		uint32_t layers = 1
	) const -> vk::Framebuffer;

	// TODO: return neutral command buffers
	auto new_command_buffers(const CommandPool &cpool, size_t count) const -> std::vector <vk::CommandBuffer>;
	auto new_descriptor_sets(const DescriptorPool &dpool, const vk::ArrayProxy <vk::DescriptorSetLayout> &dsls) const -> std::vector <vk::DescriptorSet>;
	
	template <typename ... Ts>
	auto new_render_pass(const Attachments &attachments, Ts ... subpasses) const {
		auto rp_info = vk::RenderPassCreateInfo()
			.setAttachments(attachments.descriptions)
			.setSubpasses(subpasses...);

		return RenderPass <Ts...> (logical.createRenderPass(rp_info));
	}

	template <auto &...refs, bool ... rs>
	[[nodiscard]] auto update_descriptors(DescriptorWritePair <refs, rs> ... dwpairs);

	void wait_for_frame(const Frame &frame, uint64_t timeout = UINT64_MAX) const;
	bool acquire_image_for_frame(Frame &frame, uint64_t timeout = UINT64_MAX) const;

	static Device from(const Session &session, vk::detail::DispatchLoaderDynamic &dld, const Info &info);
};
