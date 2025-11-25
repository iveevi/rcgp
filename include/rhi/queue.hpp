#pragma once

#include <vulkan/vulkan.hpp>

#include "device.hpp"

struct Queue : vk::Queue {
	uint32_t family_index;
	uint32_t queue_index;

	static Queue from(const Device &device) {
		// TODO: queue info struct with family flags..
		Queue result(device.logical.getQueue(0, 0));
		result.family_index = 0;
		result.queue_index = 0;
		return result;
	}
};
