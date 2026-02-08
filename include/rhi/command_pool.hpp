#pragma once

#include "device.hpp"
#include "queue.hpp"

namespace rcgp {

struct CommandPool : vk::CommandPool {
	static CommandPool from(const Device &device, const Queue &queue);
};

} // namespace rcgp
