#pragma once

#include "../rhi/image.hpp"

namespace rcgp {

// TODO: need to template by dimension, etc
struct MirrorSampler {
	vk::Sampler sampler {};
	vk::ImageView view {};
	vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal;

	vk::DescriptorImageInfo descriptor_info() const {
		return vk::DescriptorImageInfo {
			.sampler = sampler,
			.imageView = view,
			.imageLayout = layout,
		};
	}

	static MirrorSampler from(
		const Device &device,
		const vk::SamplerCreateInfo &sinfo,
		const Image &image,
		vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal
	) {
		MirrorSampler sm;
		sm.sampler = device.logical.createSampler(sinfo);
		sm.view = image.view;
		sm.layout = (image.layout != vk::ImageLayout::eUndefined)
			? image.layout
			: layout;
		return sm;
	}
};

} // namespace rcgp
