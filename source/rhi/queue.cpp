#include "rhi/queue.hpp"

#include <vector>

namespace rcgp {

void Queue::submit(
	const vk::ArrayProxy <vk::CommandBuffer> &cmds,
	const vk::ArrayProxy <vk::Semaphore> &wait,
	const vk::ArrayProxy <vk::Semaphore> &signal,
	const vk::PipelineStageFlags &wait_stage,
	const vk::Fence &fence
) const
{
	auto wait_stages = std::vector<vk::PipelineStageFlags>(wait.size(), wait_stage);
	vk::SubmitInfo info {};
	info.commandBufferCount = cmds.size();
	info.pCommandBuffers = cmds.data();
	info.waitSemaphoreCount = wait.size();
	info.pWaitSemaphores = wait.data();
	info.pWaitDstStageMask = wait_stages.data();
	info.signalSemaphoreCount = signal.size();
	info.pSignalSemaphores = signal.data();

	vk::Queue::submit(info, fence);
}

vk::Result Queue::present(
	const vk::SwapchainKHR &swapchain,
	uint32_t index,
	const vk::ArrayProxy <vk::Semaphore> &semaphores
) const
{
	vk::PresentInfoKHR info {};
	info.swapchainCount = 1;
	info.pSwapchains = &swapchain;
	info.pImageIndices = &index;
	info.waitSemaphoreCount = semaphores.size();
	info.pWaitSemaphores = semaphores.data();

	return presentKHR(info);
}

} // namespace rcgp
