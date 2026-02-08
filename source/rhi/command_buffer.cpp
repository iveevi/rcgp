#include <cstdlib>
#include <iostream>
#include <print>

#include "rhi/command_buffer.hpp"

namespace rcgp {

constexpr auto layout_to_stage_and_access(vk::ImageLayout layout)
	-> std::pair <vk::PipelineStageFlags2, vk::AccessFlags2>
{
	using enum vk::ImageLayout;

	switch (layout) {
	case eTransferSrcOptimal:
		return {
			vk::PipelineStageFlagBits2::eTransfer,
			vk::AccessFlagBits2::eTransferRead
		};
	case eTransferDstOptimal:
		return {
			vk::PipelineStageFlagBits2::eTransfer,
			vk::AccessFlagBits2::eTransferWrite
		};
	case eColorAttachmentOptimal:
		return {
			vk::PipelineStageFlagBits2::eColorAttachmentOutput,
			vk::AccessFlagBits2::eColorAttachmentWrite
		};
	case eDepthStencilAttachmentOptimal:
		return {
			vk::PipelineStageFlagBits2::eEarlyFragmentTests
			| vk::PipelineStageFlagBits2::eLateFragmentTests,
			vk::AccessFlagBits2::eDepthStencilAttachmentWrite
		};
	case ePresentSrcKHR:
		return {
			vk::PipelineStageFlagBits2::eBottomOfPipe,
			vk::AccessFlagBits2::eNone
		};
	case eShaderReadOnlyOptimal:
		return {
			vk::PipelineStageFlagBits2::eFragmentShader,
			vk::AccessFlagBits2::eShaderRead
		};
	default:
		return {
			vk::PipelineStageFlagBits2::eNone,
			vk::AccessFlagBits2::eNone
		};
	}
}

constexpr auto decompose_layout_transition(
	vk::ImageLayout old_layout,
	vk::ImageLayout new_layout
)
{
	// NOTE: three parts to this
	// 1. check that old_layout and new_layout are compatible
	// 2. old_layout -> src stage/access flags
	// 3. new_layout -> dst stage/access flags
	// TODO: need to check compatibility
	auto [src_stage, src_access] = layout_to_stage_and_access(old_layout);
	auto [dst_stage, dst_access] = layout_to_stage_and_access(new_layout);
	return std::tuple(src_stage, src_access, dst_stage, dst_access);
}

CommandBuffer::CommandBuffer(
	const vk::CommandBuffer &cmd,
	const vk::detail::DispatchLoaderDynamic *loader
)
	: vk::CommandBuffer(cmd), loader(loader) {}

const CommandBuffer &CommandBuffer::begin() const
{
	super::begin(vk::CommandBufferBeginInfo());
	return *this;
}

const CommandBuffer &CommandBuffer::begin(const vk::CommandBufferBeginInfo &info) const
{
	super::begin(info);
	return *this;
}

const CommandBuffer &CommandBuffer::transition_image_layout(Image &image, vk::ImageLayout new_layout) const
{
	auto [
		src_stage, src_access,
		dst_stage, dst_access
	] = decompose_layout_transition(image.layout, new_layout);

	vk::ImageMemoryBarrier2 barrier {};
	barrier.image = image.handle;
	barrier.oldLayout = image.layout;
	barrier.newLayout = new_layout;
	barrier.srcStageMask = src_stage;
	barrier.dstStageMask = dst_stage;
	barrier.srcAccessMask = src_access;
	barrier.dstAccessMask = dst_access;
	barrier.subresourceRange = image.range();

	vk::DependencyInfo dependency {};
	dependency.imageMemoryBarrierCount = 1;
	dependency.pImageMemoryBarriers = &barrier;
	pipelineBarrier2(dependency);

	image.layout = new_layout;

	return *this;
}

const CommandBuffer &CommandBuffer::copy_buffer_to_image(const Buffer &staging, const Image &image) const
{
	vk::BufferImageCopy copy {};
	copy.imageSubresource = image.layers();
	copy.imageExtent = image.extent();

	super::copyBufferToImage(staging.handle, image.handle, image.layout, copy);

	return *this;
}

const CommandBuffer &CommandBuffer::copy_image(const Image &src, const Image &dst) const
{
	vk::ImageCopy copy {};
	copy.srcSubresource = src.layers();
	copy.dstSubresource = dst.layers();
	copy.extent = src.extent();

	super::copyImage(
		src.handle,
		src.layout,
		dst.handle,
		dst.layout,
		copy
	);

	return *this;
}

const CommandBuffer &CommandBuffer::end() const
{
	super::end();
	return *this;
}

const CommandBuffer &CommandBuffer::draw_mesh_tasks(uint32_t x, uint32_t y, uint32_t z) const
{
	if (loader == nullptr) {
		std::println(std::cerr, "draw_mesh_tasks requires a dynamic loader");
		std::abort();
	}
	loader->vkCmdDrawMeshTasksEXT(*this, x, y, z);
	return *this;
}

} // namespace rcgp
