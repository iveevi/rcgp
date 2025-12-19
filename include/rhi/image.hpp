#pragma once

#include <vulkan/vulkan.hpp>

#include "device.hpp"
#include "../util/logging.hpp"

struct Image {
	struct Info {
		vk::Extent2D extent {};
		vk::Format format = vk::Format::eUndefined;
		vk::ImageUsageFlags usage {};
		vk::MemoryPropertyFlags properties {};
		vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
		vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor;
	};

	vk::Device device;
	vk::Image handle;
	vk::DeviceMemory backing;
	vk::ImageView view;
	vk::ImageLayout layout = vk::ImageLayout::eUndefined;
	Info info;

	void destroy();

	static Image from(const Device &device, const Info &info);
};
