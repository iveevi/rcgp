#pragma once

#include <tuple>
#include <vulkan/vulkan.hpp>

#include "glfw.hpp"

VKAPI_ATTR VKAPI_CALL vk::Bool32 validation_callback
(
	vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
	vk::DebugUtilsMessageTypeFlagsEXT types,
	const vk::DebugUtilsMessengerCallbackDataEXT *data,
	void *_
);

struct Session {
	vk::Instance handle;
	vk::DebugUtilsMessengerEXT debugger;

	// Session construction
	struct Info {
		bool validation = true;
		bool validation_bootstrap = true;
	};

	static std::tuple <Session, vk::detail::DispatchLoaderDynamic> from(const Info &info);
};
