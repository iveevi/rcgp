#include <vector>

#include <cstdlib>
#include <iostream>
#include <print>

#include "rhi/session.hpp"
#include "rhi/glfw.hpp"

namespace rcgp {

VKAPI_ATTR VKAPI_CALL
VkBool32 general_validation_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT types,
	const VkDebugUtilsMessengerCallbackDataEXT *data,
	void *user_data
)
{
	auto *context = reinterpret_cast <Session *> (user_data);

	if (context->validation_callback)
		context->validation_callback.value()(severity, data->pMessage);
	else
		std::println(std::cerr, "vulkan: {}", data->pMessage);

	bool trap = context->trap_on_error && (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);
	if (trap)
		std::abort();

	return VK_FALSE;
}

auto Session::from(const Options &options) -> std::tuple <
	std::unique_ptr <Session>,
	DispatchLoader
>
{
	auto product = std::tuple {
		std::make_unique <Session> (),
		DispatchLoader(),
	};

	auto &[session, dld] = product;

	glfw::boot();

	auto vkGetInstanceProcAddr = ::vkGetInstanceProcAddr;

	dld.init(vkGetInstanceProcAddr);

	auto layers = std::vector <const char *> {};
	if (options.validation)
		layers.emplace_back("VK_LAYER_KHRONOS_validation");

	auto extensions = std::vector <const char *> {
		VK_KHR_SURFACE_EXTENSION_NAME,
	};

	glfw::load_extensions(extensions);
	if (options.validation) {
		extensions.emplace_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
		extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	auto debug_severity_flags =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

	auto debug_type_flags =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	session->trap_on_error = options.trap_on_error;
	session->validation_callback = options.validation_callback;
	VkDebugUtilsMessengerCreateInfoEXT debug_info {};
	debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debug_info.messageType = debug_type_flags;
	debug_info.messageSeverity = debug_severity_flags;
	debug_info.pfnUserCallback = general_validation_callback;
	debug_info.pUserData = session.get();

	VkApplicationInfo app_info {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.apiVersion = VK_API_VERSION_1_4;
	app_info.applicationVersion = options.application_version;
	app_info.pApplicationName = options.application_name;
	app_info.engineVersion = options.engine_version;
	app_info.pEngineName = options.engine_name;

	VkValidationFeaturesEXT validation_features {};
	validation_features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
	validation_features.enabledValidationFeatureCount = options.validation_features.size();
	validation_features.pEnabledValidationFeatures = options.validation_features.data();

	VkInstanceCreateInfo instance_info {};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = &app_info;
	instance_info.enabledLayerCount = layers.size();
	instance_info.ppEnabledLayerNames = layers.data();
	instance_info.enabledExtensionCount = extensions.size();
	instance_info.ppEnabledExtensionNames = extensions.data();

	if (options.validation) {
		instance_info.pNext = &validation_features;
		if (options.validate_instance)
			validation_features.pNext = &debug_info;
	}

	auto instance = VkInstance(VK_NULL_HANDLE);
	auto iresult = vkCreateInstance(&instance_info, nullptr, &instance);
	if (iresult != VK_SUCCESS) {
		std::println(std::cerr, "failed to create vulkan instance: {}", static_cast <int> (iresult));
		std::abort();
	}
	session->handle = instance;

	dld.init(session->handle, vkGetInstanceProcAddr);

	if (options.validation && dld.vkCreateDebugUtilsMessengerEXT) {
		auto debugger = VkDebugUtilsMessengerEXT(VK_NULL_HANDLE);
		auto result = dld.vkCreateDebugUtilsMessengerEXT(
			session->handle,
			&debug_info,
			nullptr,
			&debugger
		);

		if (result == VK_SUCCESS)
			session->debugger = debugger;
	}

	return product;
}

} // namespace rcgp
