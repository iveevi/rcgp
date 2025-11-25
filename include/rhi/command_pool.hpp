#pragma once

#include <vulkan/vulkan.hpp>

#include "device.hpp"
#include "queue.hpp"

struct CommandPool : vk::CommandPool {
	static CommandPool from(const Device &device, const Queue &queue) {
		auto cpool_info = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(queue.family_index)
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

		return CommandPool(device.logical.createCommandPool(cpool_info));
	}
};
