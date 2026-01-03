#include "rhi/queue.hpp"

void Queue::submit(
	const vk::ArrayProxy <vk::CommandBuffer> &cmds,
	const vk::ArrayProxy <vk::Semaphore> &wait,
	const vk::ArrayProxy <vk::Semaphore> &signal,
	const vk::PipelineStageFlags &wait_stage,
	const vk::Fence &fence
) const
{
	auto info = vk::SubmitInfo()
		.setCommandBuffers(cmds)
		.setWaitSemaphores(wait)
		.setSignalSemaphores(signal)
		.setWaitDstStageMask(wait_stage);

	info.waitSemaphoreCount = wait.size();
	info.signalSemaphoreCount = signal.size();

	vk::Queue::submit(info, fence);
}

vk::Result Queue::present(
	const vk::SwapchainKHR &swapchain,
	uint32_t index,
	const vk::ArrayProxy <vk::Semaphore> &semaphores
) const
{
	auto info = vk::PresentInfoKHR()
		.setSwapchains(swapchain)
		.setImageIndices(index)
		.setWaitSemaphores(semaphores);

	return presentKHR(info);
}
