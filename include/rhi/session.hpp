#pragma once

#include <vulkan/vulkan.hpp>

#include <fmt/printf.h>
#include <fmt/color.h>

#include "glfw.hpp"

VKAPI_ATTR VKAPI_CALL vk::Bool32 validation_callback
(
	vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
	vk::DebugUtilsMessageTypeFlagsEXT types,
	const vk::DebugUtilsMessengerCallbackDataEXT *data,
	void *_
)
{
	auto fg = fmt::fg(fmt::color::gray);
	switch (severity) {
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
		fg = fmt::fg(fmt::color::red);
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
		fg = fmt::fg(fmt::color::yellow);
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
		fg = fmt::fg(fmt::color::blue);
		break;
	default:
		break;
	}

	auto header = fmt::format(fmt::emphasis::bold | fg, "[vvl]");
	auto message = fmt::format(fmt::emphasis::conceal, "{}", data->pMessage);

	fmt::println(stderr, "{} {}", header, message);
	return false;
}

struct Session {
	vk::Instance handle;
	vk::DebugUtilsMessengerEXT debugger;

	// Session construction
	struct Info {
		bool validation = true;
		bool validation_bootstrap = true;
	};

	static auto from(const Info &info) {
		Session session;

		glfw::boot();

		// Infoure dynamic dispatch loader
		vk::detail::DynamicLoader dl;
		auto vkGetInstanceProcAddr = dl.getProcAddress
			<PFN_vkGetInstanceProcAddr> ("vkGetInstanceProcAddr");

		vk::detail::DispatchLoaderDynamic dld;
		dld.init(vkGetInstanceProcAddr);

		// Infoure layers
		auto layers = std::vector <const char *> {};
		if (info.validation)
			layers.emplace_back("VK_LAYER_KHRONOS_validation");

		// Infoure extensions
		auto extensions = std::vector <const char *> {
			VK_KHR_SURFACE_EXTENSION_NAME,
		};

		glfw::load_extensions(extensions);
		if (info.validation) {
			extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		// Infoure the validation debugger
		auto debug_severity_flags = // Everything...
			vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
			| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
			| vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;

		auto debug_type_flags = // Everything...
			vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
			| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
			| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

		auto debug_info = vk::DebugUtilsMessengerCreateInfoEXT()
			.setMessageType(debug_type_flags)
			.setMessageSeverity(debug_severity_flags)
			.setPfnUserCallback(validation_callback);

		// Finally, instance creation
		auto app_info = vk::ApplicationInfo()
			.setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
			.setApiVersion(VK_API_VERSION_1_4)
			.setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
			.setPApplicationName("ugp")
			.setPEngineName("ugp");

		auto instance_info = vk::InstanceCreateInfo()
			.setPApplicationInfo(&app_info)
			.setPEnabledLayerNames(layers)
			.setPEnabledExtensionNames(extensions);

		if (info.validation && info.validation_bootstrap)
			instance_info.setPNext(&debug_info);

		session.handle = vk::createInstance(instance_info, nullptr, dld);

		dld.init(session.handle, vkGetInstanceProcAddr);

		// If needed infoure the validation layer messenger
		if (info.validation)
			session.debugger = session.handle.createDebugUtilsMessengerEXT(debug_info, nullptr, dld);

		// TODO: also return the dynamic loader prepped with the instance...
		return std::make_tuple(session, dld);
	}
};
