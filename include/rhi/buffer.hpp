#pragma once

#include <span>

#include "device.hpp"

namespace rcgp {

struct Buffer {
	VkDevice device = VK_NULL_HANDLE;
	VkBuffer handle = VK_NULL_HANDLE;
	VkDeviceMemory backing = VK_NULL_HANDLE;
	VkDeviceSize offset = 0;
	VkDeviceSize size = 0;
	VkBufferUsageFlags usage = 0;
	VkMemoryPropertyFlags properties = 0;

	auto write(
		const void *data,
		size_t bytes,
		VkDeviceSize relative_offset = 0
	) const -> const Buffer &;

	auto read(
		void *data,
		size_t bytes,
		VkDeviceSize relative_offset = 0
	) const -> const Buffer &;
	
	template <typename U>
	auto &write(std::span <const U> memory, VkDeviceSize offset = 0) const {
		return write(memory.data(), memory.size_bytes(), offset);
	}

	template <typename U>
	auto &read(std::span <U> memory, VkDeviceSize offset = 0) const {
		return read(memory.data(), memory.size_bytes(), offset);
	}

	VkDescriptorBufferInfo descriptor_info() const {
		VkDescriptorBufferInfo result {};
		result.buffer = handle;
		result.offset = offset;
		result.range = size;
		return result;
	}
	
	void destroy();

	static auto from(
		const Device &device,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties
	) -> Buffer;
};

// TODO: arena allocator for buffers
// TODO: interop with vma allocator?

} // namespace rcgp
