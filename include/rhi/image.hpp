#pragma once

#include "device.hpp"

namespace rcgp {

struct Image {
	struct Description {
		VkExtent3D extent {};
		VkFormat format = VK_FORMAT_UNDEFINED;
		VkImageUsageFlags usage = 0;
		VkMemoryPropertyFlags properties = 0;
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
		VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;
	};

	VkDevice device = VK_NULL_HANDLE;
	VkImage handle = VK_NULL_HANDLE;
	VkDeviceMemory backing = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
	Description description;

	VkExtent3D extent() const;
	VkImageSubresourceLayers layers() const;
	VkImageSubresourceRange range() const;
	VkDescriptorImageInfo descriptor_info(const VkSampler &sampler) const;
	void destroy();

	static Image from(const Device &device, const Description &info);
};

} // namespace rcgp
