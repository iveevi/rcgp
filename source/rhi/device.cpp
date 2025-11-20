#include "rhi/device.hpp"

#include "util/logging.hpp"

Device Device::from(const Session &session, vk::detail::DispatchLoaderDynamic &dld, const Info &info)
{
	Device device;

	device.physical = session.handle.enumeratePhysicalDevices().front();
	device.properties = device.physical.getMemoryProperties();

	auto priority = 1.0f;

	auto device_queues_info = vk::DeviceQueueCreateInfo()
		.setQueueFamilyIndex(0)
		.setQueuePriorities(priority)
		.setQueueCount(1);

	auto device_info = vk::DeviceCreateInfo()
		.setQueueCreateInfos(device_queues_info)
		.setPEnabledExtensionNames(info.extensions);

	device.logical = device.physical.createDevice(device_info);

	dld.init(device.logical);

	return device;
}

uint32_t Device::find_memory_type(uint32_t filter, vk::MemoryPropertyFlags flags) const
{
	for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
		bool type_supported = filter & (1u << i);
		bool properties_supported = (properties.memoryTypes[i].propertyFlags & flags) == flags;
		if (type_supported && properties_supported)
			return i;
	}

	fatal("no compatible memory type for buffer");
}
