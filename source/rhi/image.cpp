#include "rhi/image.hpp"

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
	const Info &info)
{
	Image result;
	result.device = device.logical;
	result.info = info;

	auto image_info = vk::ImageCreateInfo()
		.setImageType(vk::ImageType::e2D)
		.setFormat(info.format)
		.setExtent(vk::Extent3D(info.extent.width, info.extent.height, 1))
		.setMipLevels(1)
		.setArrayLayers(1)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setTiling(info.tiling)
		.setUsage(info.usage)
		.setSharingMode(vk::SharingMode::eExclusive)
		.setInitialLayout(result.layout);

	result.handle = device.logical.createImage(image_info);

	auto requirements = device.logical.getImageMemoryRequirements(result.handle);
	auto memory_index = device.find_memory_type(requirements.memoryTypeBits, info.properties);
	auto memory_info = vk::MemoryAllocateInfo()
		.setAllocationSize(requirements.size)
		.setMemoryTypeIndex(memory_index);

	result.backing = device.logical.allocateMemory(memory_info);
	device.logical.bindImageMemory(result.handle, result.backing, 0);

	auto view_info = vk::ImageViewCreateInfo()
		.setImage(result.handle)
		.setViewType(vk::ImageViewType::e2D)
		.setSubresourceRange(
			vk::ImageSubresourceRange()
				.setAspectMask(info.aspect)
				.setBaseArrayLayer(0)
				.setBaseMipLevel(0)
				.setLayerCount(1)
				.setLevelCount(1)
		)
		.setFormat(info.format);

	result.view = device.logical.createImageView(view_info);

	return result;
}
