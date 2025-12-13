#pragma once

#include <vulkan/vulkan.hpp>

#include "device.hpp"

struct Queue : vk::Queue {
	uint32_t family_index;
	uint32_t queue_index;

	auto submit(const vk::ArrayProxy <vk::CommandBuffer> &cmds,
		    const vk::ArrayProxy <vk::Semaphore> &wait,
	     	    const vk::ArrayProxy <vk::Semaphore> &signal,
	     	    const vk::PipelineStageFlags &wait_stage,
	     	    const vk::Fence &fence) const {
		auto info = vk::SubmitInfo()
			.setCommandBuffers(cmds)
			.setWaitSemaphores(wait)
			.setSignalSemaphores(signal)
			.setWaitDstStageMask(wait_stage);

		return vk::Queue::submit(info, fence);
	}

	// TODO: overload for list of pairs of swapchains and image indices
	auto present(const vk::SwapchainKHR &swapchain,
		     uint32_t index,
		     const vk::ArrayProxy <vk::Semaphore> &semaphores) const {
		auto info = vk::PresentInfoKHR()
			.setSwapchains(swapchain)
			.setImageIndices(index)
			.setWaitSemaphores(semaphores);

		return presentKHR(info);
	}

	static Queue from(const Device &device) {
		// TODO: queue info struct with family flags..
		Queue result(device.logical.getQueue(0, 0));
		result.family_index = 0;
		result.queue_index = 0;
		return result;
	}
};
