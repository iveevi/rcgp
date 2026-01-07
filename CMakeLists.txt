cmake_minimum_required(VERSION 4.0)

project(rcgp CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_LINKER_TYPE MOLD)

find_package(Vulkan REQUIRED)
find_package(glslang REQUIRED)

# GLM external import
if (NOT DEFINED RCGP_GLM_PATH)
	message(FATAL_ERROR "RCGP_GLM_PATH must point to the directory containing glm headers")
endif()

set(RCGP_GLM_PATH_RELATIVE "${RCGP_GLM_PATH}")
cmake_path(RELATIVE_PATH RCGP_GLM_PATH_RELATIVE
	BASE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")

message(STATUS "RCGP GLM path provided ${RCGP_GLM_PATH}")
include_directories("${RCGP_GLM_PATH_RELATIVE}")
include_directories(include)

add_library(rcgp STATIC
	source/dsl/instructions.cpp
	source/dsl/generators_assembly.cpp
	source/dsl/generators_glsl.cpp
	source/dsl/primitives.cpp
	source/util/logging.cpp
	source/rhi/buffer.cpp
	source/rhi/command_pool.cpp
	source/rhi/shader_compiler.cpp
	source/rhi/device.cpp
	source/rhi/descriptor_pool.cpp
	source/rhi/image.cpp
	source/rhi/session.cpp
	source/rhi/glfw.cpp
	source/rhi/queue.cpp
	source/rhi/render_pass.cpp
	source/rhi/window.cpp
)

target_precompile_headers(rcgp PRIVATE <vulkan/vulkan.hpp>)
target_link_libraries(rcgp PUBLIC
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
target_link_libraries(test PRIVATE rcgp)
