cmake_minimum_required(VERSION 4.0)

project(ugp CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_LINKER_TYPE MOLD)

find_package(Vulkan REQUIRED)

add_compile_options(-ftime-trace)

include_directories(. include)

add_executable(prototype samples/prototype.cpp)
add_executable(traditional samples/traditional.cpp)

target_precompile_headers(prototype PRIVATE <vulkan/vulkan.hpp>)
target_precompile_headers(traditional PRIVATE <vulkan/vulkan.hpp>)

target_link_libraries(prototype fmt glfw Vulkan::Vulkan)
target_link_libraries(traditional fmt glfw Vulkan::Vulkan)
