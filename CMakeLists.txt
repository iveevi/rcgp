cmake_minimum_required(VERSION 4.0)

project(rcgp CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_LINKER_TYPE MOLD)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
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

set(RCGP_GENERATED_HEADERS
	${CMAKE_CURRENT_LIST_DIR}/include/dsl/pygen_jems_instantiations.hpp
	${CMAKE_CURRENT_LIST_DIR}/include/dsl/pygen_intrinsics_instantiations.hpp
	${CMAKE_CURRENT_LIST_DIR}/include/dsl/pygen_arithmetic_instantiations.hpp
)

set(RCGP_GENERATED_SOURCES
	${CMAKE_CURRENT_LIST_DIR}/source/dsl/pygen_jems_instantiations.cpp
	${CMAKE_CURRENT_LIST_DIR}/source/dsl/pygen_intrinsics_arithmetic.cpp
)

add_custom_command(
	OUTPUT ${RCGP_GENERATED_HEADERS} ${RCGP_GENERATED_SOURCES}
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/include/dsl/generate_instantiations.py
	DEPENDS ${CMAKE_CURRENT_LIST_DIR}/include/dsl/generate_instantiations.py
	COMMENT "Generating DSL instantiation headers/sources"
)

add_custom_target(rcgp_generate_instantiations
	DEPENDS ${RCGP_GENERATED_HEADERS} ${RCGP_GENERATED_SOURCES}
)

set(RCGP_SWIZZLE_HEADER
	${CMAKE_CURRENT_LIST_DIR}/include/dsl/pygen_macro_swizzle.hpp
)

add_custom_command(
	OUTPUT ${RCGP_SWIZZLE_HEADER}
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/include/dsl/generate_macro_swizzle.py ${RCGP_SWIZZLE_HEADER}
	DEPENDS ${CMAKE_CURRENT_LIST_DIR}/include/dsl/generate_macro_swizzle.py
	COMMENT "Generating DSL swizzle macros"
)

add_custom_target(rcgp_generate_swizzle
	DEPENDS ${RCGP_SWIZZLE_HEADER}
)

add_library(rcgp STATIC
	${RCGP_GENERATED_SOURCES}
	source/dsl/instructions.cpp
	source/dsl/branching.cpp
	source/dsl/looping.cpp
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

add_dependencies(rcgp
	rcgp_generate_instantiations
	rcgp_generate_swizzle
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

target_precompile_headers(test REUSE_FROM rcgp)
target_link_libraries(test PRIVATE rcgp)
