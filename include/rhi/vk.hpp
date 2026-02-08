#pragma once

#include <vulkan/vulkan.h>

namespace rcgp {

// Minimal C-level dispatch table used for extension function entry points.
struct DispatchLoader {
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
	PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = nullptr;

	PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = nullptr;
	PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;
	PFN_vkCmdDrawMeshTasksEXT vkCmdDrawMeshTasksEXT = nullptr;

	void init(PFN_vkGetInstanceProcAddr gipa)
	{
		vkGetInstanceProcAddr = gipa;
	}

	void init(VkInstance instance, PFN_vkGetInstanceProcAddr gipa)
	{
		vkGetInstanceProcAddr = gipa;
		if (vkGetInstanceProcAddr == nullptr)
			return;

		vkGetDeviceProcAddr = reinterpret_cast <PFN_vkGetDeviceProcAddr> (
			vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr")
		);

		vkCreateDebugUtilsMessengerEXT =
			reinterpret_cast <PFN_vkCreateDebugUtilsMessengerEXT> (
				vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")
			);

		vkDestroyDebugUtilsMessengerEXT =
			reinterpret_cast <PFN_vkDestroyDebugUtilsMessengerEXT> (
				vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")
			);
	}

	void init(VkDevice device)
	{
		if (vkGetDeviceProcAddr == nullptr)
			return;

		vkCmdDrawMeshTasksEXT = reinterpret_cast <PFN_vkCmdDrawMeshTasksEXT> (
			vkGetDeviceProcAddr(device, "vkCmdDrawMeshTasksEXT")
		);
	}
};

} // namespace rcgp
