#include "rhi/command_pool.hpp"

namespace rcgp {

CommandPool CommandPool::from(const Device &device, const Queue &queue)
{
	vk::CommandPoolCreateInfo cpool_info {};
	cpool_info.queueFamilyIndex = queue.family_index;
	cpool_info.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;

	return CommandPool(device.logical.createCommandPool(cpool_info));
}

} // namespace rcgp
