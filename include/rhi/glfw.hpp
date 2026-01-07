#pragma once

#include <vector>

#include <GLFW/glfw3.h>

namespace glfw {

void boot();

void load_extensions(std::vector <const char *> &list);

} // namespace glfw
