#pragma once

#include <vulkan/vulkan.hpp>

namespace rcgp {

template <bool Live, typename ... Effects>
struct Commands;

struct Window;

struct Queue : vk::Queue {
	uint32_t family_index;
	uint32_t queue_index;

	void submit(
		const vk::ArrayProxy <vk::CommandBuffer> &cmds,
		const vk::ArrayProxy <vk::Semaphore> &wait,
		const vk::ArrayProxy <vk::Semaphore> &signal,
		const vk::PipelineStageFlags &wait_stage,
		const vk::Fence &fence
	) const;

	// Submit a live command module. Finalizes the module's recording state
	// (vkEndCommandBuffer if it is currently recording) before submission.
	template <typename ... Effects>
	void submit(
		Commands <true, Effects...> &mod,
		const vk::ArrayProxy <vk::Semaphore> &wait,
		const vk::ArrayProxy <vk::Semaphore> &signal,
		const vk::PipelineStageFlags &wait_stage,
		const vk::Fence &fence
	) const {
		auto handle = mod.finalize();
		submit(handle, wait, signal, wait_stage, fence);
	}

	// TODO: overload for list of pairs of swapchains and image indices
	bool present(
		Window &window,
		uint32_t index,
		const vk::ArrayProxy <vk::Semaphore> &semaphores
	) const;
};

} // namespace rcgp
