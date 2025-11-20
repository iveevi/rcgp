#include <cstring>
#include <utility>

#include "rhi/buffer.hpp"

void Buffer::upload(const void *data, size_t bytes, vk::DeviceSize relative_offset) const
{
	if (relative_offset + bytes > size)
		fatal("buffer upload exceeds allocation ({} + {} > {})", relative_offset, bytes, size);

	auto mapped = device.logical.mapMemory(backing, offset + relative_offset, bytes);
	std::memcpy(mapped, data, bytes);
	device.logical.unmapMemory(backing);
}

void Buffer::destroy()
{
	if (handle) {
		device.logical.destroyBuffer(handle);
		handle = nullptr;
	}

	if (backing) {
		device.logical.freeMemory(backing);
		backing = nullptr;
	}

	offset = 0;
	size = 0;
}

Buffer Buffer::from(const Device &device, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
{
	Buffer result;
	result.device = device;
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

BufferArena::BufferArena(Buffer backing, vk::DeviceSize alignment_)
	: buffer(std::move(backing))
	, alignment(alignment_)
	{}

Buffer BufferArena::allocate(vk::DeviceSize bytes)
{
	auto align_up = [](vk::DeviceSize value, vk::DeviceSize alignment) {
		return (value + alignment - 1) & ~(alignment - 1);
	};

	auto start = align_up(head, alignment);
	auto end = start + bytes;

	if (end > buffer.size)
		fatal("arena allocation exceeded buffer size ({} > {})", end, buffer.size);

	head = end;

	Buffer slice = buffer;
	slice.offset = buffer.offset + start;
	slice.size = bytes;

	slices.push_back(slice);
	return slice;
}
