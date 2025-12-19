#pragma once

#include <tuple>
#include <vulkan/vulkan.hpp>

VKAPI_ATTR VKAPI_CALL vk::Bool32 validation_callback
(
	vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
	vk::DebugUtilsMessageTypeFlagsEXT types,
	const vk::DebugUtilsMessengerCallbackDataEXT *data,
	void *user_data
);

struct Session {
	vk::Instance handle;
	vk::DebugUtilsMessengerEXT debugger;
	bool trap_on_error = true;

	// Session construction
	struct Info {
		const std::string &application_name;
		uint32_t application_version = VK_MAKE_VERSION(0, 1, 0);
		const std::string &engine_name;
		uint32_t engine_version = VK_MAKE_VERSION(0, 1, 0);
		bool validation = true;
		bool validate_instance = true;
		bool trap_on_error = true;
	};

	static std::tuple <Session, vk::detail::DispatchLoaderDynamic> from(const Info &info);
};
