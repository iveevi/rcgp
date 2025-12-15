#pragma once

#include <vulkan/vulkan.hpp>

#include "device.hpp"

struct Queue : vk::Queue {
	uint32_t family_index;
	uint32_t queue_index;

	void submit(const vk::ArrayProxy <vk::CommandBuffer> &cmds,
		  const vk::ArrayProxy <vk::Semaphore> &wait,
	      	  const vk::ArrayProxy <vk::Semaphore> &signal,
	      	  const vk::PipelineStageFlags &wait_stage,
	      	  const vk::Fence &fence) const;

	// TODO: overload for list of pairs of swapchains and image indices
	vk::Result present(const vk::SwapchainKHR &swapchain,
		     uint32_t index,
		     const vk::ArrayProxy <vk::Semaphore> &semaphores) const;

	static Queue from(const Device &device);
};
