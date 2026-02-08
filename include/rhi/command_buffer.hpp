#pragma once

#include "image.hpp"
#include "buffer.hpp"

namespace rcgp {

struct CommandBuffer : vk::CommandBuffer {
	using super = vk::CommandBuffer;

	const vk::detail::DispatchLoaderDynamic *loader = nullptr;

	CommandBuffer() = default;
	CommandBuffer(
		const vk::CommandBuffer &cmd,
		const vk::detail::DispatchLoaderDynamic *loader = nullptr
	);

	const CommandBuffer &begin() const;
	const CommandBuffer &begin(const vk::CommandBufferBeginInfo &info) const;
	const CommandBuffer &transition_image_layout(Image &image, vk::ImageLayout new_layout) const;
	const CommandBuffer &copy_buffer_to_image(const Buffer &staging, const Image &image) const;
	const CommandBuffer &copy_image(const Image &src, const Image &dst) const;
	const CommandBuffer &end() const;
	const CommandBuffer &draw_mesh_tasks(uint32_t x, uint32_t y = 1, uint32_t z = 1) const;
};

} // namespace rcgp
