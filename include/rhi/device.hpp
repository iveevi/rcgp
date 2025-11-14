#pragma once

#include "session.hpp"
#include "../msv/stage.hpp"

struct Device {
	vk::Device logical;
	vk::PhysicalDevice physical;
	vk::PhysicalDeviceMemoryProperties properties;
	vk::detail::DispatchLoaderDynamic loader;

	// Device construction
	struct Info {
		std::vector <const char *> extensions;
	};

	static auto from(const Session &session, vk::detail::DispatchLoaderDynamic &dld, const Info &info) {
		Device device;

		// Allocate the physical device
		device.physical = session.handle.enumeratePhysicalDevices().front();
		device.properties = device.physical.getMemoryProperties();

		// Allocate the logical device
		// TODO: more complex queue structuring...
		auto priority = 1.0f;

		auto device_queues_info = vk::DeviceQueueCreateInfo()
			.setQueueFamilyIndex(0)
			.setQueuePriorities(priority)
			.setQueueCount(1);

		auto device_info = vk::DeviceCreateInfo()
			.setQueueCreateInfos(device_queues_info)
			.setPEnabledExtensionNames(info.extensions);

		device.logical = device.physical.createDevice(device_info);

		// Attach to dynamic loader
		dld.init(device.logical);

		return device;
	}
};

// Shader stage compilation
struct Compiler {
	const vk::Device &reference;

	struct Info {
		// ...
	};

	static Compiler from(const Device &device, const Info &info) {
		Compiler result(device.logical);

		return result;
	}
};
