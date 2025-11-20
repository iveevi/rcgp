#pragma once

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace glfw {

inline void boot()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

inline void load_extensions(std::vector <const char *> &list)
{
	uint32_t count;
	const char **extensions = glfwGetRequiredInstanceExtensions(&count);
	list.insert(list.end(), extensions, extensions + count);
}

} // namespace glfw
