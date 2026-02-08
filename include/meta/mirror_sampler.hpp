#pragma once

#include <cstdlib>

#include "../rhi/image.hpp"

namespace rcgp {

// TODO: need to template by dimension, etc
struct MirrorSampler {
	VkSampler sampler = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	VkDescriptorImageInfo descriptor_info() const {
		VkDescriptorImageInfo info {};
		info.sampler = sampler;
		info.imageView = view;
		info.imageLayout = layout;
		return info;
	}

	static MirrorSampler from(
		const Device &device,
		const VkSamplerCreateInfo &sinfo,
		const Image &image,
		VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	) {
		MirrorSampler sm;
		auto result = vkCreateSampler(device.logical, &sinfo, nullptr, &sm.sampler);
		if (result != VK_SUCCESS)
			std::abort();
		sm.view = image.view;
		sm.layout = (image.layout != VK_IMAGE_LAYOUT_UNDEFINED)
			? image.layout
			: layout;
		return sm;
	}
};

} // namespace rcgp
