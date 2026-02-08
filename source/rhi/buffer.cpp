#include <cstring>

#include <cstdlib>
#include <iostream>
#include <print>

#include "rhi/buffer.hpp"

namespace rcgp {

auto Buffer::write(const void *data, size_t bytes, vk::DeviceSize relative_offset) const
	-> const Buffer &
{
	if (relative_offset + bytes > size) {
		std::println(std::cerr, "buffer upload exceeds allocation ({} + {} > {})",
			relative_offset, bytes, size);
		std::abort();
	}

	auto mapped = device.mapMemory(backing, offset + relative_offset, bytes);
	std::memcpy(mapped, data, bytes);
	device.unmapMemory(backing);

	return *this;
}

auto Buffer::read(void *data, size_t bytes, vk::DeviceSize relative_offset) const
	-> const Buffer &
{
	if (relative_offset + bytes > size) {
		std::println(std::cerr, "buffer read exceeds allocation ({} + {} > {})",
			relative_offset, bytes, size);
		std::abort();
	}

	auto mapped = device.mapMemory(backing, offset + relative_offset, bytes);
	std::memcpy(data, mapped, bytes);
	device.unmapMemory(backing);

	return *this;
}

void Buffer::destroy()
{
	if (handle) {
		device.destroyBuffer(handle);
		handle = nullptr;
	}

	if (backing) {
		device.freeMemory(backing);
		backing = nullptr;
	}

	offset = 0;
	size = 0;
}

auto Buffer::from(
	const Device &device,
	vk::DeviceSize size,
	vk::BufferUsageFlags usage,
	vk::MemoryPropertyFlags properties
) -> Buffer
{
	Buffer result;
	result.device = device.logical;
	result.offset = 0;
	result.size = size;
	result.usage = usage;
	result.properties = properties;

	vk::BufferCreateInfo buffer_info {};
	buffer_info.sharingMode = vk::SharingMode::eExclusive;
	buffer_info.size = size;
	buffer_info.usage = usage;

	result.handle = device.logical.createBuffer(buffer_info);

	auto requirements = device.logical.getBufferMemoryRequirements(result.handle);
	auto memory_index = device.find_memory_type(requirements.memoryTypeBits, properties);
	vk::MemoryAllocateInfo memory_info {};
	memory_info.allocationSize = requirements.size;
	memory_info.memoryTypeIndex = memory_index;

	result.backing = device.logical.allocateMemory(memory_info);
	device.logical.bindBufferMemory(result.handle, result.backing, 0);

	return result;
}

} // namespace rcgp
