cmake_minimum_required(VERSION 4.0)

project(rcgp CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_LINKER_TYPE MOLD)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Vulkan REQUIRED)
find_package(glslang REQUIRED)

include_directories(include)

set(RCGP_GENERATED
	${CMAKE_CURRENT_LIST_DIR}/include/dsl/pygen_jems_instantiations.hpp
	${CMAKE_CURRENT_LIST_DIR}/include/dsl/pygen_intrinsics_instantiations.hpp
	${CMAKE_CURRENT_LIST_DIR}/include/dsl/pygen_arithmetic_instantiations.hpp
	${CMAKE_CURRENT_LIST_DIR}/include/dsl/pygen_macro_swizzle.hpp
	${CMAKE_CURRENT_LIST_DIR}/source/dsl/pygen_jems_instantiations.cpp
	${CMAKE_CURRENT_LIST_DIR}/source/dsl/pygen_intrinsics_arithmetic.cpp
	${CMAKE_CURRENT_LIST_DIR}/source/dsl/pygen_instruction_enums.cpp
)

add_custom_command(
	OUTPUT ${RCGP_GENERATED}
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/include/dsl/generate_instantiations.py
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/include/dsl/generate_macro_swizzle.py
		${CMAKE_CURRENT_LIST_DIR}/include/dsl/pygen_macro_swizzle.hpp
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/include/dsl/generate_instruction_enums.py
	DEPENDS ${CMAKE_CURRENT_LIST_DIR}/include/dsl/generate_instantiations.py
		${CMAKE_CURRENT_LIST_DIR}/include/dsl/generate_macro_swizzle.py
		${CMAKE_CURRENT_LIST_DIR}/include/dsl/generate_instruction_enums.py
		${CMAKE_CURRENT_LIST_DIR}/include/dsl/enumerations.hpp
		COMMENT "Running Python header/source generator scripts"
)

add_custom_target(rcgp_generated
	DEPENDS ${RCGP_GENERATED}
)

file(GLOB_RECURSE RCGP_LIBRARY_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_LIST_DIR}/source/dsl/*.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/source/rhi/*.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/source/util/*.cpp"
)

# Primary library
add_library(rcgp STATIC
	${RCGP_LIBRARY_SOURCES}
	${RCGP_GENERATED}
)
add_dependencies(rcgp rcgp_generated)
target_precompile_headers(rcgp PRIVATE <vulkan/vulkan.hpp>)
target_link_libraries(rcgp PUBLIC
	fmt
	glfw
	Vulkan::Vulkan
	glslang::glslang
	glslang::glslang-default-resource-limits
)

add_executable(test EXCLUDE_FROM_ALL
	tests/dsl/main.cpp
	tests/dsl/common.cpp
	tests/dsl/recording.cpp
	tests/dsl/tracing.cpp
	tests/dsl/glsl.cpp
)
target_link_libraries(test rcgp)
