#pragma once

#include <vulkan/vulkan.hpp>

namespace rcgp {

struct Device;

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

	auto extent() const -> vk::Extent3D;
	auto layers() const -> vk::ImageSubresourceLayers;
	auto range() const -> vk::ImageSubresourceRange;
	vk::DescriptorImageInfo descriptor_info(const vk::Sampler &sampler) const;
	void destroy();
	
	vk::DescriptorImageInfo descriptor_info() const {
		return vk::DescriptorImageInfo()
			.setImageView(view)
			.setImageLayout(layout);
	}

	static Image from(const Device &device, const Description &info);
};

struct ColorTargetImage : Image {
	static ColorTargetImage from(
		const Device &device,
		const vk::Extent2D &extent,
		vk::Format format,
		vk::ImageUsageFlags extra_usage = {}
	);
};

struct DepthTargetImage : Image {
	static DepthTargetImage from(
		const Device &device,
		const vk::Extent2D &extent,
		vk::Format format,
		vk::ImageUsageFlags extra_usage = {}
	);
};

} // namespace rcgp
