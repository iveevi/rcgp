#include "rhi/glfw.hpp"

namespace glfw {

void boot()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

void load_extensions(std::vector <const char *> &list)
{
	uint32_t count;
	const char **extensions = glfwGetRequiredInstanceExtensions(&count);
	list.insert(list.end(), extensions, extensions + count);
}

} // namespace glfw
