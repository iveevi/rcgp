#include <ranges>
#include <set>

#include "rhi/command_pool.hpp"
#include "rhi/descriptor_pool.hpp"
#include "rhi/device.hpp"
#include "rhi/window.hpp"

#include "util/logging.hpp"

auto Device::find_memory_type(uint32_t filter, vk::MemoryPropertyFlags flags) const
	-> uint32_t
{
	for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
		bool type_supported = filter & (1u << i);
		bool properties_supported = (properties.memoryTypes[i].propertyFlags & flags) == flags;
		if (type_supported && properties_supported)
			return i;
	}

	fatal("no compatible memory type for buffer");
}

auto Device::new_command_buffers(const CommandPool &cpool, size_t count) const
	-> std::vector <vk::CommandBuffer>
{
	auto info = vk::CommandBufferAllocateInfo()
		.setCommandBufferCount(count)
		.setCommandPool(cpool);

	return logical.allocateCommandBuffers(info);
}
	
auto Device::new_descriptor_sets(const DescriptorPool &dpool, const vk::ArrayProxy <vk::DescriptorSetLayout> &dsls) const
	-> std::vector <vk::DescriptorSet>
{
	auto info = vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(dpool)
		.setSetLayouts(dsls);

	return logical.allocateDescriptorSets(info);
}
	
void Device::wait_for_frame(const Frame &frame, uint64_t timeout) const
{
	auto result = logical.waitForFences(frame.fence, true, timeout);
	logical.resetFences(frame.fence);
}

bool Device::acquire_image_for_frame(Frame &frame, uint64_t timeout) const
{
	auto acq = logical.acquireNextImageKHR(
		frame.swapchain,
		timeout,
		frame.presented,
		nullptr
	);

	frame.image_index = acq.value;

	return !(acq.result == vk::Result::eErrorOutOfDateKHR || acq.result == vk::Result::eSuboptimalKHR);
}

Device Device::from(
	const Session &session,
	vk::detail::DispatchLoaderDynamic &dld,
	const Options &options
)
{
	Device device;

	device.physical = session.handle.enumeratePhysicalDevices().front();
	device.properties = device.physical.getMemoryProperties();

	auto queue_families = device.physical.getQueueFamilyProperties();

	info("queue families:");
	for (const auto &[i, family] : std::views::enumerate(queue_families)) {
		info("%zu: %u of %s",
			static_cast<size_t>(i),
			family.queueCount,
			vk::to_string(family.queueFlags).c_str());
	}

	auto queue_allocation = std::set <int32_t> ();
	auto queue_family_index = [&](const vk::QueueFlags &flags) -> int32_t {
		// Prefer exact match
		for (const auto &[i, family] : std::views::enumerate(queue_families)) {
			if (family.queueFlags == flags)
				return i;
		}
		
		// Otherwise, prefer first non-allocated one
		int32_t first_applicable = -1;
		for (const auto &[i, family] : std::views::enumerate(queue_families)) {
			if (family.queueFlags & flags) {
				if (first_applicable < 0)
					first_applicable = i;
				
				if (not queue_allocation.contains(i))
					return i;
			}
		}

		// Otherwise, prefer the first applicable
		return first_applicable;
	};

	const auto priority = 1.0f;
	auto queue_create_infos = std::vector <vk::DeviceQueueCreateInfo> ();
	queue_create_infos.reserve(options.queues.size());

	for (auto &[key, flags] : options.queues) {
		auto idx = queue_family_index(flags);
		if (idx == -1) {
			error("failed to find a suitable queue for '%s'", key);
			continue;
		}

		queue_create_infos.emplace_back(
			vk::DeviceQueueCreateInfo()
				.setQueueFamilyIndex(idx)
				.setQueuePriorities(priority)
				.setQueueCount(1)
		);

		device.queues.emplace(key, Queue(nullptr, idx, 0));
	}

	auto features13 = vk::PhysicalDeviceVulkan13Features()
		.setSynchronization2(true);

	auto device_info = vk::DeviceCreateInfo()
		.setQueueCreateInfos(queue_create_infos)
		.setPEnabledExtensionNames(options.extensions)
		.setPNext(&features13);

	device.logical = device.physical.createDevice(device_info);

	dld.init(device.logical);

	// Populate the queue handles
	for (auto &[_, queue] : device.queues)
		static_cast <vk::Queue &> (queue) = device.logical.getQueue(queue.family_index, 0);

	return device;
}
