cmake_minimum_required(VERSION 4.0)

project(ugp CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_LINKER_TYPE MOLD)

find_package(Vulkan REQUIRED)

set(ALLOW_EXTERNAL_SPIRV_TOOLS TRUE)
add_subdirectory(ext/glslang EXCLUDE_FROM_ALL)

include_directories(include ext/glslang ext/glm)

add_library(ugp STATIC
	source/rhi/buffer.cpp
	source/rhi/command_pool.cpp
	source/rhi/compiler.cpp
	source/rhi/device.cpp
	source/rhi/descriptor_pool.cpp
	source/rhi/image.cpp
	source/rhi/session.cpp
	source/rhi/glfw.cpp
	source/rhi/queue.cpp
	source/rhi/render_pass.cpp
	source/rhi/window.cpp
)

target_link_libraries(ugp PUBLIC
	fmt
	glfw
	Vulkan::Vulkan
	glslang::glslang
	glslang::glslang-default-resource-limits
)

add_executable(test EXCLUDE_FROM_ALL
	source/test/main.cpp
	source/test/mirror/std430.cpp
	source/test/mirror/scalar.cpp
)

target_precompile_headers(test PRIVATE <vulkan/vulkan.hpp>)
target_link_libraries(test PRIVATE ugp mrd)
