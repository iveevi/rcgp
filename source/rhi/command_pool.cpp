#include "rhi/command_pool.hpp"

CommandPool CommandPool::from(const Device &device, const Queue &queue)
{
	auto cpool_info = vk::CommandPoolCreateInfo()
		.setQueueFamilyIndex(queue.family_index)
		.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

	return CommandPool(device.logical.createCommandPool(cpool_info));
}
