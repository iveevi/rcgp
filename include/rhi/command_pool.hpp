#pragma once

#include "device.hpp"
#include "queue.hpp"

namespace rcgp {

struct CommandPool {
	VkCommandPool handle = VK_NULL_HANDLE;

	CommandPool() = default;
	CommandPool(VkCommandPool handle) : handle(handle) {}

	operator VkCommandPool() const
	{
		return handle;
	}

	static CommandPool from(const Device &device, const Queue &queue);
};

} // namespace rcgp
