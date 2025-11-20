#pragma once

#include <vector>

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

	static Device from(const Session &session, vk::detail::DispatchLoaderDynamic &dld, const Info &info);

	uint32_t find_memory_type(uint32_t filter, vk::MemoryPropertyFlags flags) const;
};
