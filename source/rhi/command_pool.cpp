#include "rhi/command_pool.hpp"

#include <cstdlib>
#include <iostream>
#include <print>

namespace rcgp {

CommandPool CommandPool::from(const Device &device, const Queue &queue)
{
	VkCommandPoolCreateInfo cpool_info {};
	cpool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cpool_info.queueFamilyIndex = queue.family_index;
	cpool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	auto handle = VkCommandPool(VK_NULL_HANDLE);
	auto result = vkCreateCommandPool(
		device.logical,
		&cpool_info,
		nullptr,
		&handle
	);
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed to create command pool: {}", static_cast <int> (result));
		std::abort();
	}

	return CommandPool(handle);
}

} // namespace rcgp
