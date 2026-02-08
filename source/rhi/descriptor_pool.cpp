#include "rhi/descriptor_pool.hpp"

#include <cstdlib>
#include <iostream>
#include <print>
#include <vector>

namespace rcgp {

DescriptorPool DescriptorPool::from(const Device &device, const Options &info)
{
	std::vector <VkDescriptorPoolSize> sizes;
	sizes.reserve(14);

	auto push = [&sizes](VkDescriptorType type, uint32_t count) {
		if (count)
			sizes.emplace_back(type, count);
	};

	push(VK_DESCRIPTOR_TYPE_SAMPLER, info.samplers);
	push(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, info.combined_image_samplers);
	push(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, info.sampled_images);
	push(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, info.storage_images);
	push(VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, info.uniform_texel_buffers);
	push(VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, info.storage_texel_buffers);
	push(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, info.uniform_buffers);
	push(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, info.storage_buffers);
	push(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, info.uniform_buffers_dynamic);
	push(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, info.storage_buffers_dynamic);
	push(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, info.input_attachments);
	push(VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT, info.inline_uniform_blocks);
	push(VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, info.acceleration_structures);

	VkDescriptorPoolCreateInfo pool_info {};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.maxSets = info.max_sets;
	pool_info.poolSizeCount = sizes.size();
	pool_info.pPoolSizes = sizes.data();

	auto handle = VkDescriptorPool(VK_NULL_HANDLE);
	auto result = vkCreateDescriptorPool(
		device.logical,
		&pool_info,
		nullptr,
		&handle
	);
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed to create descriptor pool: {}", static_cast <int> (result));
		std::abort();
	}

	return DescriptorPool(handle);
}

} // namespace rcgp
