cmake_minimum_required(VERSION 4.0)

project(rcgp CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_LINKER_TYPE MOLD)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Vulkan REQUIRED)
find_package(glslang REQUIRED)

include_directories(include)

set(RCGP_GENERATED_SWIZZLE
	${CMAKE_CURRENT_LIST_DIR}/include/dsl/pygen_macro_swizzle.hpp
)
add_custom_command(
	OUTPUT ${RCGP_GENERATED_SWIZZLE}
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/include/dsl/generate_macro_swizzle.py
	DEPENDS ${CMAKE_CURRENT_LIST_DIR}/include/dsl/generate_macro_swizzle.py
	COMMENT "Running generate_macro_swizzle.py..."
)
add_custom_target(rcgp_generate_macro_swizzle DEPENDS ${RCGP_GENERATED_SWIZZLE})

set(RCGP_GENERATED_ENUM
	${CMAKE_CURRENT_LIST_DIR}/source/dsl/pygen_enumeration_repr.cpp
	${CMAKE_CURRENT_LIST_DIR}/source/dsl/pygen_enumeration_repr_glsl.cpp
)
add_custom_command(
	OUTPUT ${RCGP_GENERATED_ENUM}
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/include/dsl/generate_enumeration_strings.py
	DEPENDS ${CMAKE_CURRENT_LIST_DIR}/include/dsl/generate_enumeration_strings.py
		${CMAKE_CURRENT_LIST_DIR}/include/dsl/enumerations.hpp
	COMMENT "Running generate_enumeration_strings.py..."
)
add_custom_target(rcgp_generate_enumeration_strings DEPENDS ${RCGP_GENERATED_ENUM})

set(RCGP_GENERATED ${RCGP_GENERATED_SWIZZLE} ${RCGP_GENERATED_ENUM})

# Primary library
file(GLOB_RECURSE RCGP_LIBRARY_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_LIST_DIR}/source/dsl/*.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/source/rhi/*.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/source/util/*.cpp"
)
add_library(rcgp STATIC ${RCGP_LIBRARY_SOURCES} ${RCGP_GENERATED})
add_dependencies(rcgp
	rcgp_generate_macro_swizzle
	rcgp_generate_enumeration_strings
)
target_precompile_headers(rcgp PRIVATE <vulkan/vulkan.hpp>)
target_link_libraries(rcgp PUBLIC
	fmt glfw Vulkan::Vulkan
	glslang::glslang glslang::glslang-default-resource-limits
)

# Runtime tests
file(GLOB_RECURSE RCGP_DSL_TESTS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/tests/dsl/*.cpp")
add_executable(test EXCLUDE_FROM_ALL ${RCGP_DSL_TESTS})
target_precompile_headers(test REUSE_FROM rcgp)
target_link_libraries(test rcgp)

# Experimental program
add_executable(experimental EXCLUDE_FROM_ALL tests/experimental.cpp)
target_precompile_headers(experimental REUSE_FROM rcgp)
target_link_libraries(experimental rcgp)
