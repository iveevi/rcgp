#include <ranges>
#include <set>

#include "rhi/command_buffer.hpp"
#include "rhi/command_pool.hpp"
#include "rhi/descriptor_pool.hpp"
#include "rhi/device.hpp"
#include "rhi/window.hpp"
#include "rhi/timestamp_pool.hpp"

#include "util/logging.hpp"
#include "util/cti.hpp"

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
	-> std::vector <CommandBuffer>
{
	auto info = vk::CommandBufferAllocateInfo()
		.setCommandBufferCount(count)
		.setCommandPool(cpool);

	return logical.allocateCommandBuffers(info)
		| std::views::transform([&] (auto x) { return CommandBuffer(x, &loader); })
		| std::ranges::to <std::vector> ();
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

TimestampQueryPool Device::new_timestamp_pool(vk::QueryResultFlags flags, size_t count) const
{
	TimestampQueryPool tqpool;

	auto info = vk::QueryPoolCreateInfo()
		.setQueryCount(count)
		.setQueryType(vk::QueryType::eTimestamp);


	auto properties = physical.getProperties();
	tqpool.period = properties.limits.timestampPeriod;
	tqpool.handle = logical.createQueryPool(info);
	tqpool.flags = flags;
	tqpool.count = count;

	return tqpool;
}

TimestampQueryResult Device::get_timestamp_results(const TimestampQueryPool &tqpool) const
{
	auto su64 = sizeof(uint64_t);
	auto stride = (tqpool.flags & vk::QueryResultFlagBits::eWithAvailability) ? (2 * su64) : su64;
	auto data_size = tqpool.count * stride;
	auto queries = logical.getQueryPoolResults <uint64_t> (
		tqpool.handle,
		0, tqpool.count,
		data_size, stride,
		vk::QueryResultFlagBits::e64 | tqpool.flags
	);

	if (queries.has_value())
		return { queries.value, tqpool.flags, tqpool.period };
	else
		return { {}, tqpool.flags, tqpool.period };
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
		info("%d: %u of %s", i, family.queueCount,
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
		.setSynchronization2(true)
		.setMaintenance4(options.maintenance4);

	void *feature_chain = nullptr;

	vk::PhysicalDeviceScalarBlockLayoutFeatures scalar_layout;
	if (options.scalar_block_layout) {
		scalar_layout.setScalarBlockLayout(true);
		scalar_layout.setPNext(feature_chain);
		feature_chain = &scalar_layout;
	}

	vk::PhysicalDeviceMeshShaderFeaturesEXT mesh_features;
	if (options.mesh_shaders) {
		mesh_features
			.setTaskShader(true)
			.setMeshShader(true);
		mesh_features.setPNext(feature_chain);
		feature_chain = &mesh_features;
	}

	features13.setPNext(feature_chain);

	auto device_info = vk::DeviceCreateInfo()
		.setQueueCreateInfos(queue_create_infos)
		.setPEnabledExtensionNames(options.extensions)
		.setPNext(&features13);

	device.logical = device.physical.createDevice(device_info);

	dld.init(device.logical);
	device.loader = dld;

	// Populate the queue handles
	for (auto &[_, queue] : device.queues)
		Tas <vk::Queue &> (queue) = device.logical.getQueue(queue.family_index, 0);

	return device;
}
