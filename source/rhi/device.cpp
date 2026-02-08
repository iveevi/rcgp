#include <cstdlib>
#include <iostream>
#include <print>
#include <set>
#include <vector>

#include "rhi/command_buffer.hpp"
#include "rhi/command_pool.hpp"
#include "rhi/descriptor_pool.hpp"
#include "rhi/device.hpp"
#include "rhi/timestamp_pool.hpp"
#include "rhi/window.hpp"

#include "util/cti.hpp"

namespace rcgp {

auto Device::find_memory_type(uint32_t filter, VkMemoryPropertyFlags flags) const
	-> uint32_t
{
	for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
		bool type_supported = (filter & (1u << i)) != 0;
		bool properties_supported = (properties.memoryTypes[i].propertyFlags & flags) == flags;
		if (type_supported && properties_supported)
			return i;
	}

	std::println(std::cerr, "no compatible memory type for buffer");
	std::abort();
}

auto Device::new_command_buffers(const CommandPool &cpool, size_t count) const
	-> std::vector <CommandBuffer>
{
	VkCommandBufferAllocateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	info.commandPool = cpool;
	info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	info.commandBufferCount = count;

	auto vk_cmds = std::vector <VkCommandBuffer> (count);
	auto result = vkAllocateCommandBuffers(
		logical,
		&info,
		vk_cmds.data()
	);
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed to allocate command buffers: {}", static_cast <int> (result));
		std::abort();
	}

	auto cmds = std::vector <CommandBuffer> {};
	cmds.reserve(vk_cmds.size());
	for (auto cmd : vk_cmds)
		cmds.emplace_back(cmd, &loader);

	return cmds;
}


auto Device::new_descriptor_sets(
	const DescriptorPool &dpool,
	const std::span <const VkDescriptorSetLayout> &dsls
) const -> std::vector <VkDescriptorSet>
{
	VkDescriptorSetAllocateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	info.descriptorPool = dpool;
	info.descriptorSetCount = dsls.size();
	info.pSetLayouts = dsls.data();

	auto dsets = std::vector <VkDescriptorSet> (dsls.size());
	auto result = vkAllocateDescriptorSets(
		logical,
		&info,
		dsets.data()
	);
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed to allocate descriptor sets: {}", static_cast <int> (result));
		std::abort();
	}

	return dsets;
}

void Device::wait_for_frame(const Frame &frame, uint64_t timeout) const
{
	auto result = vkWaitForFences(
		logical,
		1,
		&frame.fence,
		VK_TRUE,
		timeout
	);
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed waiting for fence: {}", static_cast <int> (result));
		std::abort();
	}

	result = vkResetFences(
		logical,
		1,
		&frame.fence
	);
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed resetting fence: {}", static_cast <int> (result));
		std::abort();
	}
}

bool Device::acquire_image_for_frame(Frame &frame, uint64_t timeout) const
{
	auto image_index = uint32_t(0);
	auto result = vkAcquireNextImageKHR(
		logical,
		frame.swapchain,
		timeout,
		frame.presented,
		VK_NULL_HANDLE,
		&image_index
	);
	frame.image_index = image_index;

	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		return false;
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed to acquire image: {}", static_cast <int> (result));
		std::abort();
	}

	return true;
}

TimestampQueryPool Device::new_timestamp_pool(VkQueryResultFlags flags, size_t count) const
{
	TimestampQueryPool tqpool;

	VkQueryPoolCreateInfo info {};
	info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	info.queryCount = count;
	info.queryType = VK_QUERY_TYPE_TIMESTAMP;

	VkPhysicalDeviceProperties physical_properties {};
	vkGetPhysicalDeviceProperties(physical, &physical_properties);
	tqpool.period = physical_properties.limits.timestampPeriod;

	auto result = vkCreateQueryPool(
		logical,
		&info,
		nullptr,
		&tqpool.handle
	);
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed to create query pool: {}", static_cast <int> (result));
		std::abort();
	}
	tqpool.flags = flags;
	tqpool.count = count;

	return tqpool;
}

TimestampQueryResult Device::get_timestamp_results(const TimestampQueryPool &tqpool) const
{
	auto su64 = sizeof(uint64_t);
	auto stride = (tqpool.flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) ? (2 * su64) : su64;
	auto data_size = tqpool.count * stride;
	auto queries = std::vector <uint64_t> (data_size / sizeof(uint64_t));
	auto result = vkGetQueryPoolResults(
		logical,
		tqpool.handle,
		0,
		tqpool.count,
		data_size,
		queries.data(),
		stride,
		VK_QUERY_RESULT_64_BIT | tqpool.flags
	);

	if (result == VK_SUCCESS)
		return { queries, tqpool.flags, tqpool.period };
	if (result == VK_NOT_READY)
		return { {}, tqpool.flags, tqpool.period };

	std::println(std::cerr, "failed to get query pool results: {}", static_cast <int> (result));
	std::abort();
}

Device Device::from(
	const Session &session,
	DispatchLoader &dld,
	const Options &options
)
{
	Device device;

	auto physical_device_count = uint32_t(0);
	auto result = vkEnumeratePhysicalDevices(
		session.handle,
		&physical_device_count,
		nullptr
	);
	if (result != VK_SUCCESS || physical_device_count == 0) {
		std::println(std::cerr, "failed to enumerate physical devices: {}", static_cast <int> (result));
		std::abort();
	}

	auto physical_devices = std::vector <VkPhysicalDevice> (physical_device_count);
	result = vkEnumeratePhysicalDevices(
		session.handle,
		&physical_device_count,
		physical_devices.data()
	);
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed to fetch physical devices: {}", static_cast <int> (result));
		std::abort();
	}
	device.physical = physical_devices.front();
	vkGetPhysicalDeviceMemoryProperties(device.physical, &device.properties);

	auto queue_family_count = uint32_t(0);
	vkGetPhysicalDeviceQueueFamilyProperties(device.physical, &queue_family_count, nullptr);
	auto queue_families = std::vector <VkQueueFamilyProperties> (queue_family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(device.physical, &queue_family_count, queue_families.data());

	auto queue_allocation = std::set <int32_t> {};

	auto queue_family_index = [&](VkQueueFlags flags) -> int32_t {
		for (size_t i = 0; i < queue_families.size(); ++i) {
			if (queue_families[i].queueFlags == flags)
				return i;
		}

		int32_t first_applicable = -1;
		for (size_t i = 0; i < queue_families.size(); ++i) {
			if ((queue_families[i].queueFlags & flags) == 0)
				continue;

			if (first_applicable < 0)
				first_applicable = i;

			if (!queue_allocation.contains(i))
				return i;
		}

		return first_applicable;
	};

	const auto priority = 1.0f;
	auto queue_create_infos = std::vector <VkDeviceQueueCreateInfo> {};
	queue_create_infos.reserve(options.queues.size());
	auto queue_create_family_indices = std::set <int32_t> {};

	for (auto &[key, flags] : options.queues) {
		auto idx = queue_family_index(flags);
		if (idx == -1) {
			std::println(std::cerr, "failed to find a suitable queue for '{}'", key);
			continue;
		}

		if (!queue_create_family_indices.contains(idx)) {
			VkDeviceQueueCreateInfo queue_info {};
			queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_info.queueFamilyIndex = idx;
			queue_info.queueCount = 1;
			queue_info.pQueuePriorities = &priority;
			queue_create_infos.emplace_back(queue_info);
			queue_create_family_indices.emplace(idx);
		}

		queue_allocation.emplace(idx);
		device.queues.emplace(key, Queue(VK_NULL_HANDLE, idx, 0));
	}

	VkPhysicalDeviceVulkan13Features features13 {};
	features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	features13.synchronization2 = VK_TRUE;
	features13.maintenance4 = options.maintenance4;

	void *feature_chain = nullptr;

	VkPhysicalDeviceScalarBlockLayoutFeatures scalar_layout {};
	scalar_layout.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES;
	if (options.scalar_block_layout) {
		scalar_layout.scalarBlockLayout = VK_TRUE;
		scalar_layout.pNext = feature_chain;
		feature_chain = &scalar_layout;
	}

	VkPhysicalDeviceMeshShaderFeaturesEXT mesh_features {};
	mesh_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
	if (options.mesh_shaders) {
		mesh_features.taskShader = VK_TRUE;
		mesh_features.meshShader = VK_TRUE;
		mesh_features.pNext = feature_chain;
		feature_chain = &mesh_features;
	}

	if (options.dynamic_rendering)
		features13.dynamicRendering = VK_TRUE;

	features13.pNext = feature_chain;

	VkDeviceCreateInfo device_info {};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.queueCreateInfoCount = queue_create_infos.size();
	device_info.pQueueCreateInfos = queue_create_infos.data();
	device_info.enabledExtensionCount = options.extensions.size();
	device_info.ppEnabledExtensionNames = options.extensions.data();
	device_info.pNext = &features13;

	result = vkCreateDevice(device.physical, &device_info, nullptr, &device.logical);
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed to create device: {}", static_cast <int> (result));
		std::abort();
	}

	dld.init(device.logical);
	device.loader = dld;

	for (auto &[_, queue] : device.queues)
		vkGetDeviceQueue(device.logical, queue.family_index, queue.queue_index, &queue.handle);

	return device;
}

} // namespace rcgp
