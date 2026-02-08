#pragma once

#include "../dsl/aliases.hpp"
#include "../rhi/vk.hpp"
#include "../util/cti.hpp"
#include "static_string.hpp"

namespace rcgp {

template <typename T>
struct symbolic_format {
	static_error("bad"_ss);
};

template <>
struct symbolic_format <vec2> : std::integral_constant <VkFormat, VK_FORMAT_R32G32_SFLOAT> {};

template <>
struct symbolic_format <vec3> : std::integral_constant <VkFormat, VK_FORMAT_R32G32B32_SFLOAT> {};

template <>
struct symbolic_format <vec4> : std::integral_constant <VkFormat, VK_FORMAT_R32G32B32A32_SFLOAT> {};

} // namespace rcgp
