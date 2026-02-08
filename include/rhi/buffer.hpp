#pragma once

#include <span>

#include "device.hpp"

namespace rcgp {

struct Buffer {
	vk::Device device;
	vk::Buffer handle;
	vk::DeviceMemory backing;
	vk::DeviceSize offset = 0;
	vk::DeviceSize size = 0;
	vk::BufferUsageFlags usage;
	vk::MemoryPropertyFlags properties;

	auto write(
		const void *data,
		size_t bytes,
		vk::DeviceSize relative_offset = 0
	) const -> const Buffer &;

	auto read(
		void *data,
		size_t bytes,
		vk::DeviceSize relative_offset = 0
	) const -> const Buffer &;
	
	template <typename U>
	auto &write(std::span <const U> memory, vk::DeviceSize offset = 0) const {
		return write(memory.data(), memory.size_bytes(), offset);
	}

	template <typename U>
	auto &read(std::span <U> memory, vk::DeviceSize offset = 0) const {
		return read(memory.data(), memory.size_bytes(), offset);
	}

	vk::DescriptorBufferInfo descriptor_info() const {
		vk::DescriptorBufferInfo result {};
		result.buffer = handle;
		result.offset = offset;
		result.range = size;
		return result;
	}
	
	void destroy();

	static auto from(
		const Device &device,
		vk::DeviceSize size,
		vk::BufferUsageFlags usage,
		vk::MemoryPropertyFlags properties
	) -> Buffer;
};

// TODO: arena allocator for buffers
// TODO: interop with vma allocator?

} // namespace rcgp
