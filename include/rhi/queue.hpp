#pragma once

#include <span>

#include "vk.hpp" 

namespace rcgp {

struct Queue {
	VkQueue handle = VK_NULL_HANDLE;
	uint32_t family_index;
	uint32_t queue_index;

	Queue() = default;
	Queue(VkQueue handle, uint32_t family_index, uint32_t queue_index)
		: handle(handle), family_index(family_index), queue_index(queue_index) {}

	operator VkQueue() const
	{
		return handle;
	}

	void submit(
		const std::span <const VkCommandBuffer> &cmds,
		const std::span <const VkSemaphore> &wait,
		const std::span <const VkSemaphore> &signal,
		const VkPipelineStageFlags &wait_stage,
		const VkFence &fence
	) const;
	void submit(
		const VkCommandBuffer &cmd,
		const VkSemaphore &wait,
		const VkSemaphore &signal,
		const VkPipelineStageFlags &wait_stage,
		const VkFence &fence
	) const;

	// TODO: overload for list of pairs of swapchains and image indices
	VkResult present(
		const VkSwapchainKHR &swapchain,
		uint32_t index,
		const std::span <const VkSemaphore> &semaphores
	) const;
	VkResult present(
		const VkSwapchainKHR &swapchain,
		uint32_t index,
		const VkSemaphore &semaphore
	) const;

	void wait_idle() const;
	void waitIdle() const { wait_idle(); }
};

} // namespace rcgp
