#include <vector>

#include <fmt/printf.h>
#include <fmt/color.h>

#include "rhi/session.hpp"
#include "rhi/glfw.hpp"

VKAPI_ATTR VKAPI_CALL vk::Bool32 validation_callback
(
	vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
	vk::DebugUtilsMessageTypeFlagsEXT types,
	const vk::DebugUtilsMessengerCallbackDataEXT *data,
	void *user_data
)
{
	const auto *session = static_cast <const Session *> (user_data);
	const bool trap_on_error = session ? session->trap_on_error : false;

	auto fg = fmt::fg(fmt::color::gray);
	bool trap = false;

	switch (severity) {
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError:
		fg = fmt::fg(fmt::color::red);
		trap = trap_on_error;
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning:
		fg = fmt::fg(fmt::color::yellow);
		break;
	case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo:
		fg = fmt::fg(fmt::color::light_blue);
		break;
	default:
		break;
	}

	auto header = fmt::format(fmt::emphasis::bold | fg, "[vvl]");
	auto message = fmt::format(fmt::emphasis::faint, "{}", data->pMessage);

	fmt::println(stderr, "{} {}", header, message);
	if (trap)
		__builtin_trap();

	return false;
}

std::tuple <Session, vk::detail::DispatchLoaderDynamic> Session::from(const Info &info)
{
	auto product = std::tuple <Session, vk::detail::DispatchLoaderDynamic> {};
	auto &[session, dld] = product;

	session.trap_on_error = info.trap_on_error;

	glfw::boot();

	vk::detail::DynamicLoader dl;
	auto vkGetInstanceProcAddr = dl.getProcAddress
		<PFN_vkGetInstanceProcAddr> ("vkGetInstanceProcAddr");

	dld.init(vkGetInstanceProcAddr);

	auto layers = std::vector <const char *> {};
	if (info.validation)
		layers.emplace_back("VK_LAYER_KHRONOS_validation");

	auto extensions = std::vector <const char *> {
		VK_KHR_SURFACE_EXTENSION_NAME,
	};

	glfw::load_extensions(extensions);
	if (info.validation) {
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

	auto debug_info = vk::DebugUtilsMessengerCreateInfoEXT()
		.setMessageType(debug_type_flags)
		.setMessageSeverity(debug_severity_flags)
		.setPfnUserCallback(validation_callback)
		.setPUserData(&session);

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

	if (info.validation)
		session.debugger = session.handle.createDebugUtilsMessengerEXT(debug_info, nullptr, dld);

	return product;
}
