#include <vector>

#include <cstdlib>
#include <iostream>
#include <print>

#include "rhi/session.hpp"
#include "rhi/glfw.hpp"

namespace rcgp {

VKAPI_ATTR VKAPI_CALL
vk::Bool32 general_validation_callback(
	vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
	vk::DebugUtilsMessageTypeFlagsEXT types,
	const vk::DebugUtilsMessengerCallbackDataEXT *data,
	void *user_data
)
{
	auto *context = reinterpret_cast <Session *> (user_data);

	if (context->validation_callback)
		context->validation_callback.value()(severity, data->pMessage);
	else
		std::println(std::cerr, "vulkan: {}", data->pMessage);

	bool trap = context->trap_on_error && (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
	if (trap)
		std::abort();

	return false;
}

auto Session::from(const Options &options) -> std::tuple <
	std::unique_ptr <Session>,
	vk::detail::DispatchLoaderDynamic
>
{
	auto product = std::tuple {
		std::make_unique <Session> (),
		vk::detail::DispatchLoaderDynamic(),
	};

	auto &[session, dld] = product;

	glfw::boot();

	vk::detail::DynamicLoader dl;
	auto vkGetInstanceProcAddr = dl.getProcAddress
		<PFN_vkGetInstanceProcAddr> ("vkGetInstanceProcAddr");

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
		vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
		| vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;

	auto debug_type_flags =
		vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
		| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
		| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;

	session->trap_on_error = options.trap_on_error;
	session->validation_callback = options.validation_callback;
	vk::DebugUtilsMessengerCreateInfoEXT debug_info {};
	debug_info.messageType = debug_type_flags;
	debug_info.messageSeverity = debug_severity_flags;
	debug_info.pfnUserCallback = general_validation_callback;
	debug_info.pUserData = session.get();

	vk::ApplicationInfo app_info {};
	app_info.apiVersion = VK_API_VERSION_1_4;
	app_info.applicationVersion = options.application_version;
	app_info.pApplicationName = options.application_name;
	app_info.engineVersion = options.engine_version;
	app_info.pEngineName = options.engine_name;

	vk::ValidationFeaturesEXT validation_features {};
	validation_features.enabledValidationFeatureCount = options.validation_features.size();
	validation_features.pEnabledValidationFeatures = options.validation_features.data();

	vk::InstanceCreateInfo instance_info {};
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

	session->handle = vk::createInstance(instance_info, nullptr, dld);

	dld.init(session->handle, vkGetInstanceProcAddr);

	if (options.validation)
		session->debugger = session->handle.createDebugUtilsMessengerEXT(debug_info, nullptr, dld);

	return product;
}

} // namespace rcgp
