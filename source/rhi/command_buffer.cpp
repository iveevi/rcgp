#include <cstdlib>
#include <iostream>
#include <print>
#include <tuple>
#include <utility>

#include "rhi/command_buffer.hpp"

namespace rcgp {

constexpr auto layout_to_stage_and_access(VkImageLayout layout)
	-> std::pair <VkPipelineStageFlags2, VkAccessFlags2>
{
	switch (layout) {
	case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
		return {
			VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_TRANSFER_READ_BIT
		};
	case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
		return {
			VK_PIPELINE_STAGE_2_TRANSFER_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT
		};
	case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
		return {
			VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT
		};
	case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
		return {
			VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT
			| VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
			VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
		};
	case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
		return {
			VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
			0
		};
	case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
		return {
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_READ_BIT
		};
	default:
		return { 0, 0 };
	}
}

constexpr auto decompose_layout_transition(
	VkImageLayout old_layout,
	VkImageLayout new_layout
)
{
	auto [src_stage, src_access] = layout_to_stage_and_access(old_layout);
	auto [dst_stage, dst_access] = layout_to_stage_and_access(new_layout);
	return std::tuple(src_stage, src_access, dst_stage, dst_access);
}

const CommandBuffer &CommandBuffer::begin() const
{
	VkCommandBufferBeginInfo info {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	return begin(info);
}

const CommandBuffer &CommandBuffer::begin(const VkCommandBufferBeginInfo &info) const
{
	auto result = vkBeginCommandBuffer(handle, &info);
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed to begin command buffer: {}", static_cast <int> (result));
		std::abort();
	}
	return *this;
}

const CommandBuffer &CommandBuffer::transition_image_layout(Image &image, VkImageLayout new_layout) const
{
	auto [
		src_stage, src_access,
		dst_stage, dst_access
	] = decompose_layout_transition(image.layout, new_layout);

	VkImageMemoryBarrier2 barrier {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
	barrier.image = image.handle;
	barrier.oldLayout = image.layout;
	barrier.newLayout = new_layout;
	barrier.srcStageMask = src_stage;
	barrier.dstStageMask = dst_stage;
	barrier.srcAccessMask = src_access;
	barrier.dstAccessMask = dst_access;
	barrier.subresourceRange = image.range();

	VkDependencyInfo dependency {};
	dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	dependency.imageMemoryBarrierCount = 1;
	dependency.pImageMemoryBarriers = &barrier;
	vkCmdPipelineBarrier2(handle, &dependency);

	image.layout = new_layout;
	return *this;
}

const CommandBuffer &CommandBuffer::copy_buffer_to_image(const Buffer &staging, const Image &image) const
{
	VkBufferImageCopy copy {};
	copy.imageSubresource = image.layers();
	copy.imageExtent = image.extent();

	vkCmdCopyBufferToImage(
		handle,
		staging.handle,
		image.handle,
		image.layout,
		1,
		&copy
	);

	return *this;
}

const CommandBuffer &CommandBuffer::copy_image(const Image &src, const Image &dst) const
{
	VkImageCopy copy {};
	copy.srcSubresource = src.layers();
	copy.dstSubresource = dst.layers();
	copy.extent = src.extent();

	vkCmdCopyImage(
		handle,
		src.handle,
		src.layout,
		dst.handle,
		dst.layout,
		1,
		&copy
	);

	return *this;
}

const CommandBuffer &CommandBuffer::end() const
{
	auto result = vkEndCommandBuffer(handle);
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed to end command buffer: {}", static_cast <int> (result));
		std::abort();
	}
	return *this;
}

const CommandBuffer &CommandBuffer::draw_mesh_tasks(uint32_t x, uint32_t y, uint32_t z) const
{
	if (loader == nullptr) {
		std::println(std::cerr, "draw_mesh_tasks requires a dynamic loader");
		std::abort();
	}
	if (loader->vkCmdDrawMeshTasksEXT == nullptr) {
		std::println(std::cerr, "vkCmdDrawMeshTasksEXT is not available");
		std::abort();
	}

	loader->vkCmdDrawMeshTasksEXT(handle, x, y, z);
	return *this;
}

} // namespace rcgp
