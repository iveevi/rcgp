#pragma once

#include "../rhi/device.hpp"
#include "../rhi/image.hpp"
#include "resources.hpp"

namespace rcgp {

// TODO: need to template by dimension, etc
struct MirrorSampler {
	vk::Sampler sampler {};
	vk::ImageView view {};
	vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal;

	vk::DescriptorImageInfo descriptor_info() const {
		return vk::DescriptorImageInfo()
			.setSampler(sampler)
			.setImageView(view)
			.setImageLayout(layout);
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

template <auto &target_ref>
requires is_render_target_v <reference_base_of <target_ref>>
struct TargetMirrorSampler {
	using image_type = std::conditional_t <
		is_depth_target_v <reference_base_of <target_ref>>,
		DepthTargetImage, ColorTargetImage
	>;

	image_type *image = nullptr;
	vk::Sampler sampler {};

	vk::DescriptorImageInfo descriptor_info() const {
		return vk::DescriptorImageInfo()
			.setSampler(sampler)
			.setImageView(image->view)
			.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
	}

	static TargetMirrorSampler from(
		const Device &device,
		image_type *img,
		const vk::SamplerCreateInfo &sinfo
	) {
		TargetMirrorSampler result;
		result.image = img;
		result.sampler = device.logical.createSampler(sinfo);
		return result;
	}
};

} // namespace rcgp
