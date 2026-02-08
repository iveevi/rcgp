#pragma once

#include <memory>
#include <functional>
#include <optional>
#include <string_view>
#include <tuple>

#include "vk.hpp"

namespace rcgp {

using ValidationCallback = std::function <void (VkDebugUtilsMessageSeverityFlagBitsEXT, std::string_view)>;

struct Session {
	VkInstance handle = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT debugger = VK_NULL_HANDLE;
	bool trap_on_error = true;
	std::optional <ValidationCallback> validation_callback;

	operator VkInstance() const
	{
		return handle;
	}

	Session() = default;
	Session(const Session &) = delete;
	Session &operator=(const Session &) = delete;

	struct Options {
		const char *const application_name;
		uint32_t application_version = VK_MAKE_VERSION(0, 1, 0);
		const char *const engine_name;
		uint32_t engine_version = VK_MAKE_VERSION(0, 1, 0);
		bool validation = true;
		bool validate_instance = true;
		bool trap_on_error = true;
		std::optional <ValidationCallback> validation_callback;
		std::vector <VkValidationFeatureEnableEXT> validation_features;
	};

	static auto from(const Options &info) -> std::tuple <
		std::unique_ptr <Session>,
		DispatchLoader
	>;
};

} // namespace rcgp
