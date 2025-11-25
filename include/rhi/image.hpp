#pragma once

#include <vulkan/vulkan.hpp>

struct Image {
	vk::Image handle;
	vk::ImageView view;
	vk::ImageLayout layout = vk::ImageLayout::eUndefined;
	vk::Extent2D extent;
	vk::Format format = vk::Format::eUndefined;

	vk::ImageMemoryBarrier memory_barrier(vk::ImageLayout new_layout, vk::AccessFlags src_access = {}, vk::AccessFlags dst_access = {}) {
		vk::ImageMemoryBarrier barrier;
		barrier.setImage(handle)
			.setOldLayout(layout)
			.setNewLayout(new_layout)
			.setSrcAccessMask(src_access)
			.setDstAccessMask(dst_access)
			.setSubresourceRange(
				vk::ImageSubresourceRange()
					.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setBaseArrayLayer(0)
					.setBaseMipLevel(0)
					.setLayerCount(1)
					.setLevelCount(1)
			);
		layout = new_layout;
		return barrier;
	}
};
