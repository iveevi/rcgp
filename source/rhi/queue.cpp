#include "rhi/queue.hpp"

#include <array>
#include <vector>

namespace rcgp {

void Queue::submit(
	const std::span <const VkCommandBuffer> &cmds,
	const std::span <const VkSemaphore> &wait,
	const std::span <const VkSemaphore> &signal,
	const VkPipelineStageFlags &wait_stage,
	const VkFence &fence
) const
{
	auto wait_stages = std::vector <VkPipelineStageFlags> (wait.size(), wait_stage);

	VkSubmitInfo info {};
	info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	info.commandBufferCount = cmds.size();
	info.pCommandBuffers = cmds.data();
	info.waitSemaphoreCount = wait.size();
	info.pWaitSemaphores = wait.data();
	info.pWaitDstStageMask = wait_stages.data();
	info.signalSemaphoreCount = signal.size();
	info.pSignalSemaphores = signal.data();

	vkQueueSubmit(handle, 1, &info, fence);
}

void Queue::submit(
	const VkCommandBuffer &cmd,
	const VkSemaphore &wait,
	const VkSemaphore &signal,
	const VkPipelineStageFlags &wait_stage,
	const VkFence &fence
) const
{
	auto cmds = std::array { cmd };
	auto waits = std::span <const VkSemaphore> ();
	auto signals = std::span <const VkSemaphore> ();
	if (wait != VK_NULL_HANDLE)
		waits = std::span <const VkSemaphore> (&wait, 1);
	if (signal != VK_NULL_HANDLE)
		signals = std::span <const VkSemaphore> (&signal, 1);
	submit(cmds, waits, signals, wait_stage, fence);
}

VkResult Queue::present(
	const VkSwapchainKHR &swapchain,
	uint32_t index,
	const std::span <const VkSemaphore> &semaphores
) const
{
	VkPresentInfoKHR info {};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.swapchainCount = 1;
	info.pSwapchains = &swapchain;
	info.pImageIndices = &index;
	info.waitSemaphoreCount = semaphores.size();
	info.pWaitSemaphores = semaphores.data();

	return vkQueuePresentKHR(handle, &info);
}

VkResult Queue::present(
	const VkSwapchainKHR &swapchain,
	uint32_t index,
	const VkSemaphore &semaphore
) const
{
	auto semaphores = std::span <const VkSemaphore> ();
	if (semaphore != VK_NULL_HANDLE)
		semaphores = std::span <const VkSemaphore> (&semaphore, 1);
	return present(swapchain, index, semaphores);
}

void Queue::wait_idle() const
{
	vkQueueWaitIdle(handle);
}

} // namespace rcgp
