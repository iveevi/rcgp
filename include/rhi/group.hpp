#pragma once

#include <cstdint>
#include <span>
#include <vulkan/vulkan.hpp>

#include "device.hpp"
#include "window.hpp"
#include "command_pool.hpp"
#include "render_pass.hpp"

struct group_device_window {
	Device &device;
	Window &window;

	void wait(Window::Frame &frame, uint64_t timeout = UINT64_MAX) {
		auto result = device.logical.waitForFences(frame.fence, true, timeout);
		device.logical.resetFences(frame.fence);
	}

	bool acquire_image(Window::Frame &frame, uint64_t timeout = UINT64_MAX) {
		auto result = device.logical.acquireNextImageKHR(
			window.swapchain,
			timeout,
			frame.presented,
			nullptr,
			&frame.image_index
		);

		return !(result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR);
	}
};

inline auto group(Device &device, Window &window)
{
	return group_device_window(device, window);
}

struct group_device_cpool {
	const Device &device;
	const CommandPool &cpool;

	auto allocate(uint32_t count) {
		return device.logical.allocateCommandBuffers(
			vk::CommandBufferAllocateInfo()
				.setCommandBufferCount(count)
				.setCommandPool(cpool)
		);
	}
};

inline auto group(const Device &device, const CommandPool &cpool)
{
	return group_device_cpool(device, cpool);
}

struct group_device_dld {
	Device &device;
	const vk::detail::DispatchLoaderDynamic &dld;

	template <typename ... Ts>
	auto new_render_pass(const Attachments &attachments, Ts ... subpasses) const {
		return ::render_pass(device, dld, attachments, subpasses...);
	}
};

inline auto group(Device &device, const vk::detail::DispatchLoaderDynamic &dld)
{
	return group_device_dld { device, dld };
}

struct group_device_render_pass {
	Device &device;
	const vk::detail::DispatchLoaderDynamic &dld;
	vk::RenderPass rp;

	auto new_framebuffer(std::span <const vk::ImageView> attachments, vk::Extent2D extent, uint32_t layers = 1) const {
		auto fb_info = vk::FramebufferCreateInfo()
			.setRenderPass(rp)
			.setAttachments(attachments)
			.setWidth(extent.width)
			.setHeight(extent.height)
			.setLayers(layers);

		return device.logical.createFramebuffer(fb_info, nullptr, dld);
	}
};

inline auto group(Device &device, const vk::RenderPass &rp, const vk::detail::DispatchLoaderDynamic &dld)
{
	return group_device_render_pass { device, dld, rp };
}
