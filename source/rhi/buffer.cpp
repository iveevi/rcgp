#include <cstring>

#include <cstdlib>
#include <iostream>
#include <print>

#include "rhi/buffer.hpp"

namespace rcgp {

auto Buffer::write(const void *data, size_t bytes, VkDeviceSize relative_offset) const
	-> const Buffer &
{
	if (relative_offset + bytes > size) {
		std::println(std::cerr, "buffer upload exceeds allocation ({} + {} > {})",
			relative_offset, bytes, size);
		std::abort();
	}

	void *mapped = nullptr;
	auto result = vkMapMemory(
		device,
		backing,
		offset + relative_offset,
		bytes,
		0,
		&mapped
	);
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed to map buffer memory: {}", static_cast <int> (result));
		std::abort();
	}
	std::memcpy(mapped, data, bytes);
	vkUnmapMemory(device, backing);

	return *this;
}

auto Buffer::read(void *data, size_t bytes, VkDeviceSize relative_offset) const
	-> const Buffer &
{
	if (relative_offset + bytes > size) {
		std::println(std::cerr, "buffer read exceeds allocation ({} + {} > {})",
			relative_offset, bytes, size);
		std::abort();
	}

	void *mapped = nullptr;
	auto result = vkMapMemory(
		device,
		backing,
		offset + relative_offset,
		bytes,
		0,
		&mapped
	);
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed to map buffer memory: {}", static_cast <int> (result));
		std::abort();
	}
	std::memcpy(data, mapped, bytes);
	vkUnmapMemory(device, backing);

	return *this;
}

void Buffer::destroy()
{
	if (handle) {
		vkDestroyBuffer(device, handle, nullptr);
		handle = nullptr;
	}

	if (backing) {
		vkFreeMemory(device, backing, nullptr);
		backing = nullptr;
	}

	offset = 0;
	size = 0;
}

auto Buffer::from(
	const Device &device,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties
) -> Buffer
{
	Buffer result;
	result.device = device.logical;
	result.offset = 0;
	result.size = size;
	result.usage = usage;
	result.properties = properties;

	VkBufferCreateInfo buffer_info {};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_info.size = size;
	buffer_info.usage = usage;

	auto vk_buffer = VkBuffer(VK_NULL_HANDLE);
	auto cresult = vkCreateBuffer(
		device.logical,
		&buffer_info,
		nullptr,
		&vk_buffer
	);
	if (cresult != VK_SUCCESS) {
		std::println(std::cerr, "failed to create buffer: {}", static_cast <int> (cresult));
		std::abort();
	}
	result.handle = vk_buffer;

	VkMemoryRequirements requirements {};
	vkGetBufferMemoryRequirements(device.logical, result.handle, &requirements);
	auto memory_index = device.find_memory_type(requirements.memoryTypeBits, properties);
	VkMemoryAllocateInfo memory_info {};
	memory_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memory_info.allocationSize = requirements.size;
	memory_info.memoryTypeIndex = memory_index;

	auto vk_memory = VkDeviceMemory(VK_NULL_HANDLE);
	cresult = vkAllocateMemory(
		device.logical,
		&memory_info,
		nullptr,
		&vk_memory
	);
	if (cresult != VK_SUCCESS) {
		std::println(std::cerr, "failed to allocate buffer memory: {}", static_cast <int> (cresult));
		std::abort();
	}
	result.backing = vk_memory;

	cresult = vkBindBufferMemory(
		device.logical,
		result.handle,
		result.backing,
		0
	);
	if (cresult != VK_SUCCESS) {
		std::println(std::cerr, "failed to bind buffer memory: {}", static_cast <int> (cresult));
		std::abort();
	}

	return result;
}

} // namespace rcgp
