#include "rhi/image.hpp"

#include <cstdlib>

namespace rcgp {

VkExtent3D Image::extent() const
{
	return description.extent;
}

VkImageSubresourceLayers Image::layers() const
{
	VkImageSubresourceLayers result {};
	result.aspectMask = description.aspect;
	result.mipLevel = 0;
	result.baseArrayLayer = 0;
	result.layerCount = 1;
	return result;
}

VkImageSubresourceRange Image::range() const
{
	VkImageSubresourceRange result {};
	result.aspectMask = description.aspect;
	result.baseArrayLayer = 0;
	result.baseMipLevel = 0;
	result.layerCount = 1;
	result.levelCount = 1;
	return result;
}

VkDescriptorImageInfo Image::descriptor_info(const VkSampler &sampler) const
{
	VkDescriptorImageInfo result {};
	result.sampler = sampler;
	result.imageView = view;
	result.imageLayout = layout;
	return result;
}

void Image::destroy()
{
	if (view) {
		vkDestroyImageView(device, view, nullptr);
		view = nullptr;
	}

	if (backing) {
		vkDestroyImage(device, handle, nullptr);
		vkFreeMemory(device, backing, nullptr);
		handle = nullptr;
		backing = nullptr;
		layout = VK_IMAGE_LAYOUT_UNDEFINED;
	}
}

Image Image::from(
	const Device &device,
	const Description &info
)
{
	Image result;
	result.device = device.logical;
	result.description = info;

	VkImageCreateInfo image_info {};
	image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image_info.imageType = VK_IMAGE_TYPE_2D;
	image_info.format = info.format;
	image_info.extent = VkExtent3D {
		info.extent.width,
		info.extent.height,
		1
	};
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.samples = VK_SAMPLE_COUNT_1_BIT;
	image_info.tiling = info.tiling;
	image_info.usage = info.usage;
	image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	image_info.initialLayout = result.layout;

	auto vk_image = VkImage(VK_NULL_HANDLE);
	auto cresult = vkCreateImage(
		device.logical,
		&image_info,
		nullptr,
		&vk_image
	);
	if (cresult != VK_SUCCESS)
		std::abort();
	result.handle = vk_image;

	VkMemoryRequirements requirements {};
	vkGetImageMemoryRequirements(device.logical, result.handle, &requirements);
	auto memory_index = device.find_memory_type(requirements.memoryTypeBits, info.properties);

	VkMemoryAllocateInfo memory_info {};
	memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_info.allocationSize = requirements.size;
	memory_info.memoryTypeIndex = memory_index;

	auto vk_memory = VkDeviceMemory(VK_NULL_HANDLE);
	cresult = vkAllocateMemory(
		device.logical,
		&memory_info,
		nullptr,
		&vk_memory
	);
	if (cresult != VK_SUCCESS)
		std::abort();
	result.backing = vk_memory;

	cresult = vkBindImageMemory(
		device.logical,
		result.handle,
		result.backing,
		0
	);
	if (cresult != VK_SUCCESS)
		std::abort();

	VkImageSubresourceRange view_range {};
	view_range.aspectMask = info.aspect;
	view_range.baseArrayLayer = 0;
	view_range.baseMipLevel = 0;
	view_range.layerCount = 1;
	view_range.levelCount = 1;

	VkImageViewCreateInfo view_info {};
	view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image = result.handle;
	view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	view_info.subresourceRange = view_range;
	view_info.format = info.format;

	auto vk_view = VkImageView(VK_NULL_HANDLE);
	cresult = vkCreateImageView(
		device.logical,
		&view_info,
		nullptr,
		&vk_view
	);
	if (cresult != VK_SUCCESS)
		std::abort();
	result.view = vk_view;

	return result;
}

} // namespace rcgp
