#pragma once

#include "device.hpp"

namespace rcgp {

struct DescriptorPool {
	VkDescriptorPool handle = VK_NULL_HANDLE;

	DescriptorPool() = default;
	DescriptorPool(VkDescriptorPool handle) : handle(handle) {}

	operator VkDescriptorPool() const
	{
		return handle;
	}

	struct Options {
		uint32_t max_sets = 0;
		uint32_t samplers = 0;
		uint32_t combined_image_samplers = 0;
		uint32_t sampled_images = 0;
		uint32_t storage_images = 0;
		uint32_t uniform_texel_buffers = 0;
		uint32_t storage_texel_buffers = 0;
		uint32_t uniform_buffers = 0;
		uint32_t storage_buffers = 0;
		uint32_t uniform_buffers_dynamic = 0;
		uint32_t storage_buffers_dynamic = 0;
		uint32_t input_attachments = 0;
		uint32_t inline_uniform_blocks = 0; // VK_EXT_inline_uniform_block
		uint32_t acceleration_structures = 0; // VK_KHR_acceleration_structure
	};

	static DescriptorPool from(const Device &device, const Options &info);
};

} // namespace rcgp
