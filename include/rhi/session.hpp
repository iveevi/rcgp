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
		bool validation = true;
		bool validation_bootstrap = true;
		bool trap_on_error = true;
	};

	static std::tuple <Session, vk::detail::DispatchLoaderDynamic> from(const Info &info);
};
