#pragma once

#include <vulkan/vulkan.hpp>

#include "device.hpp"
#include "queue.hpp"

struct CommandPool : vk::CommandPool {
	static CommandPool from(const Device &device, const Queue &queue);
};
