#include "rhi/presentation_synchronizer.hpp"
#include "rhi/device.hpp"

namespace rcgp {

vk::Semaphore PresentationSynchronizer::acquire_semaphore() const
{
	return acquire_semaphores[current_slot];
}

vk::Fence PresentationSynchronizer::fence() const
{
	return fences[current_slot];
}

vk::Semaphore PresentationSynchronizer::render_semaphore() const
{
	return render_semaphores[current_image];
}

PresentationSynchronizer PresentationSynchronizer::from(const Device &device, size_t count)
{
	PresentationSynchronizer sync;
	auto fence_info = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);
	auto sem_info = vk::SemaphoreCreateInfo();
	sync.fences.resize(count);
	sync.acquire_semaphores.resize(count);
	sync.render_semaphores.resize(count);
	for (size_t i = 0; i < count; i++) {
		sync.fences[i] = device.logical.createFence(fence_info);
		sync.acquire_semaphores[i] = device.logical.createSemaphore(sem_info);
		sync.render_semaphores[i] = device.logical.createSemaphore(sem_info);
	}
	sync.current_slot = count > 0 ? (count - 1) : 0;  // first advance lands on slot 0
	return sync;
}

} // namespace rcgp
