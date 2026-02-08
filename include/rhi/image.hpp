#pragma once

#include "device.hpp"

namespace rcgp {

struct Image {
	struct Description {
		vk::Extent3D extent {};
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
	Description description;

	vk::Extent3D extent() const;
	vk::ImageSubresourceLayers layers() const;
	vk::ImageSubresourceRange range() const;
	vk::DescriptorImageInfo descriptor_info(const vk::Sampler &sampler) const;
	void destroy();

	static Image from(const Device &device, const Description &info);
};

} // namespace rcgp
