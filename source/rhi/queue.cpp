#include "rhi/queue.hpp"
#include "rhi/window.hpp"

namespace rcgp {

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

bool Queue::present(
	Window &window,
	uint32_t index,
	const vk::ArrayProxy <vk::Semaphore> &semaphores
) const
{
	auto info = vk::PresentInfoKHR()
		.setSwapchains(window.swapchain)
		.setImageIndices(index)
		.setWaitSemaphores(semaphores);

	auto result = vkQueuePresentKHR(
		static_cast <VkQueue> (*this),
		reinterpret_cast <const VkPresentInfoKHR *> (&info)
	);

	if (result == VK_ERROR_OUT_OF_DATE_KHR or result == VK_SUBOPTIMAL_KHR) {
		window.is_swapchain_out_of_date = true;
		return false;
	}

	return true;
}

} // namespace rcgp
