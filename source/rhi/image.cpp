#include "rhi/image.hpp"

namespace rcgp {

vk::Extent3D Image::extent() const
{
	return description.extent;
}

vk::ImageSubresourceLayers Image::layers() const
{
	vk::ImageSubresourceLayers result {};
	result.aspectMask = description.aspect;
	result.mipLevel = 0;
	result.baseArrayLayer = 0;
	result.layerCount = 1;
	return result;
}

vk::ImageSubresourceRange Image::range() const
{
	vk::ImageSubresourceRange result {};
	result.aspectMask = description.aspect;
	result.baseArrayLayer = 0;
	result.baseMipLevel = 0;
	result.layerCount = 1;
	result.levelCount = 1;
	return result;
}

vk::DescriptorImageInfo Image::descriptor_info(const vk::Sampler &sampler) const
{
	vk::DescriptorImageInfo result {};
	result.sampler = sampler;
	result.imageView = view;
	result.imageLayout = layout;
	return result;
}

void Image::destroy()
{
	if (view) {
		device.destroyImageView(view);
		view = nullptr;
	}

	if (backing) {
		device.destroyImage(handle);
		device.freeMemory(backing);
		handle = nullptr;
		backing = nullptr;
		layout = vk::ImageLayout::eUndefined;
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

	vk::ImageCreateInfo image_info {};
	image_info.imageType = vk::ImageType::e2D;
	image_info.format = info.format;
	image_info.extent = vk::Extent3D(info.extent.width, info.extent.height, 1);
	image_info.mipLevels = 1;
	image_info.arrayLayers = 1;
	image_info.samples = vk::SampleCountFlagBits::e1;
	image_info.tiling = info.tiling;
	image_info.usage = info.usage;
	image_info.sharingMode = vk::SharingMode::eExclusive;
	image_info.initialLayout = result.layout;

	result.handle = device.logical.createImage(image_info);

	auto requirements = device.logical.getImageMemoryRequirements(result.handle);
	auto memory_index = device.find_memory_type(requirements.memoryTypeBits, info.properties);

	vk::MemoryAllocateInfo memory_info {};
	memory_info.allocationSize = requirements.size;
	memory_info.memoryTypeIndex = memory_index;

	result.backing = device.logical.allocateMemory(memory_info);
	device.logical.bindImageMemory(result.handle, result.backing, 0);

	vk::ImageSubresourceRange view_range {};
	view_range.aspectMask = info.aspect;
	view_range.baseArrayLayer = 0;
	view_range.baseMipLevel = 0;
	view_range.layerCount = 1;
	view_range.levelCount = 1;

	vk::ImageViewCreateInfo view_info {};
	view_info.image = result.handle;
	view_info.viewType = vk::ImageViewType::e2D;
	view_info.subresourceRange = view_range;
	view_info.format = info.format;

	result.view = device.logical.createImageView(view_info);

	return result;
}

} // namespace rcgp
