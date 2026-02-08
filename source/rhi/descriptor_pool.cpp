#include "rhi/descriptor_pool.hpp"

#include <vector>

namespace rcgp {

DescriptorPool DescriptorPool::from(const Device &device, const Options &info)
{
	std::vector <vk::DescriptorPoolSize> sizes;
	sizes.reserve(14);

	auto push = [&sizes](vk::DescriptorType type, uint32_t count) {
		if (count)
			sizes.emplace_back(type, count);
	};

	push(vk::DescriptorType::eSampler, info.samplers);
	push(vk::DescriptorType::eCombinedImageSampler, info.combined_image_samplers);
	push(vk::DescriptorType::eSampledImage, info.sampled_images);
	push(vk::DescriptorType::eStorageImage, info.storage_images);
	push(vk::DescriptorType::eUniformTexelBuffer, info.uniform_texel_buffers);
	push(vk::DescriptorType::eStorageTexelBuffer, info.storage_texel_buffers);
	push(vk::DescriptorType::eUniformBuffer, info.uniform_buffers);
	push(vk::DescriptorType::eStorageBuffer, info.storage_buffers);
	push(vk::DescriptorType::eUniformBufferDynamic, info.uniform_buffers_dynamic);
	push(vk::DescriptorType::eStorageBufferDynamic, info.storage_buffers_dynamic);
	push(vk::DescriptorType::eInputAttachment, info.input_attachments);
	push(vk::DescriptorType::eInlineUniformBlockEXT, info.inline_uniform_blocks);
	push(vk::DescriptorType::eAccelerationStructureKHR, info.acceleration_structures);

	vk::DescriptorPoolCreateInfo pool_info {};
	pool_info.maxSets = info.max_sets;
	pool_info.poolSizeCount = sizes.size();
	pool_info.pPoolSizes = sizes.data();

	return DescriptorPool(device.logical.createDescriptorPool(pool_info));
}

} // namespace rcgp
