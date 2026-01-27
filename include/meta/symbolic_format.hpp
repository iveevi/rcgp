#pragma once

#include "static_string.hpp"
#include "../dsl/aliases.hpp"
#include "../util/cti.hpp"

namespace rcgp {

template <typename T>
struct symbolic_format {
	static_error("bad"_ss);
};

template <>
struct symbolic_format <vec2> : std::integral_constant <vk::Format, vk::Format::eR32G32Sfloat> {};

template <>
struct symbolic_format <vec3> : std::integral_constant <vk::Format, vk::Format::eR32G32B32Sfloat> {};

template <>
struct symbolic_format <vec4> : std::integral_constant <vk::Format, vk::Format::eR32G32B32A32Sfloat> {};

} // namespace rcgp
