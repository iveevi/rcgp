#include "rhi/image.hpp"
#include "rhi/device.hpp"

namespace rcgp {

auto Image::extent() const -> vk::Extent3D
{
	return description.extent;
}

auto Image::layers() const -> vk::ImageSubresourceLayers
{
	return vk::ImageSubresourceLayers()
		.setAspectMask(description.aspect)
		.setMipLevel(0)
		.setBaseArrayLayer(0)
		.setLayerCount(1);
}

auto Image::range() const -> vk::ImageSubresourceRange
{
	return vk::ImageSubresourceRange()
		.setAspectMask(description.aspect)
		.setBaseArrayLayer(0)
		.setBaseMipLevel(0)
		.setLayerCount(1)
		.setLevelCount(1);
}

vk::DescriptorImageInfo Image::descriptor_info(const vk::Sampler &sampler) const
{
	return vk::DescriptorImageInfo()
		.setSampler(sampler)
		.setImageView(view)
		.setImageLayout(layout);
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

ColorTargetImage ColorTargetImage::from(
	const Device &device,
	const vk::Extent2D &extent,
	vk::Format format,
	vk::ImageUsageFlags extra_usage
)
{
	auto desc = Image::Description {
		.extent = vk::Extent3D(extent.width, extent.height, 1),
		.format = format,
		.usage = vk::ImageUsageFlagBits::eColorAttachment
		       | vk::ImageUsageFlagBits::eSampled
		       | extra_usage,
		.properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
		.tiling = vk::ImageTiling::eOptimal,
		.aspect = vk::ImageAspectFlagBits::eColor,
	};

	ColorTargetImage result;
	static_cast <Image &> (result) = Image::from(device, desc);
	return result;
}

DepthTargetImage DepthTargetImage::from(
	const Device &device,
	const vk::Extent2D &extent,
	vk::Format format,
	vk::ImageUsageFlags extra_usage
)
{
	auto desc = Image::Description {
		.extent = vk::Extent3D(extent.width, extent.height, 1),
		.format = format,
		.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment
		       | vk::ImageUsageFlagBits::eSampled
		       | extra_usage,
		.properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
		.tiling = vk::ImageTiling::eOptimal,
		.aspect = vk::ImageAspectFlagBits::eDepth,
	};

	DepthTargetImage result;
	static_cast <Image &> (result) = Image::from(device, desc);
	return result;
}

} // namespace rcgp
