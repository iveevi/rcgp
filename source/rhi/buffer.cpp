#include <cstring>

#include "util/logging.hpp"
#include "rhi/buffer.hpp"

auto Buffer::write(const void *data, size_t bytes, vk::DeviceSize relative_offset) const
	-> const Buffer &
{
	if (relative_offset + bytes > size)
		fatal("buffer upload exceeds allocation (%llu + %zu > %llu)",
			static_cast<unsigned long long>(relative_offset),
			bytes,
			static_cast<unsigned long long>(size));

	auto mapped = device.mapMemory(backing, offset + relative_offset, bytes);
	std::memcpy(mapped, data, bytes);
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

	auto buffer_info = vk::BufferCreateInfo()
		.setSharingMode(vk::SharingMode::eExclusive)
		.setSize(size)
		.setUsage(usage);

	result.handle = device.logical.createBuffer(buffer_info);

	auto requirements = device.logical.getBufferMemoryRequirements(result.handle);
	auto memory_index = device.find_memory_type(requirements.memoryTypeBits, properties);
	auto memory_info = vk::MemoryAllocateInfo()
		.setAllocationSize(requirements.size)
		.setMemoryTypeIndex(memory_index);

	result.backing = device.logical.allocateMemory(memory_info);
	device.logical.bindBufferMemory(result.handle, result.backing, 0);

	return result;
}
