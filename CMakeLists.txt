cmake_minimum_required(VERSION 4.0)

project(ugp CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_LINKER_TYPE MOLD)

find_package(Vulkan REQUIRED)

add_compile_options(-ftime-trace)

set(ALLOW_EXTERNAL_SPIRV_TOOLS TRUE)
set(MRD_GLM_PATH "${CMAKE_CURRENT_LIST_DIR}/ext/glm")

add_subdirectory(ext/glslang EXCLUDE_FROM_ALL)
add_subdirectory(ext/mrd EXCLUDE_FROM_ALL)

include_directories(include ext/glslang ext/glm ext/mrd)

add_library(ugp STATIC
	source/rhi/buffer.cpp
	source/rhi/compiler.cpp
	source/rhi/device.cpp
	source/rhi/image.cpp
	source/rhi/session.cpp
	source/rhi/window.cpp
)

target_link_libraries(ugp PUBLIC
	fmt
	glfw
	Vulkan::Vulkan
	glslang::glslang
	glslang::glslang-default-resource-limits
)

add_executable(prototype samples/prototype.cpp)
add_executable(rasterization samples/rasterization.cpp)

add_executable(test
	source/test/main.cpp
	source/test/mirror/std430.cpp
	source/test/mirror/scalar.cpp)

target_precompile_headers(test PRIVATE <vulkan/vulkan.hpp>)
target_precompile_headers(prototype REUSE_FROM test)
target_precompile_headers(rasterization REUSE_FROM test)

target_link_libraries(test PRIVATE ugp mrd)
target_link_libraries(prototype PRIVATE ugp mrd)
target_link_libraries(rasterization PRIVATE ugp mrd)
