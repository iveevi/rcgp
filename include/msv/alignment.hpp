#pragma once

#include <cstddef>
#include <glm/glm.hpp>

#include "../dsl/primitives.hpp"

constexpr size_t align_up(size_t value, size_t alignment)
{
	return (value + alignment - 1) & ~(alignment - 1);
}

template <typename T>
struct alignment {};

template <native_scalar T>
struct alignment <T> {
	static constexpr size_t value = 4;
};

template <native_scalar T>
struct alignment <glm::tvec2 <T, glm::qualifier::defaultp>> {
	static constexpr size_t value = 8;
};

template <native_scalar T>
struct alignment <glm::tvec3 <T, glm::qualifier::defaultp>> {
	static constexpr size_t value = 16;
};

template <typename T, size_t N>
struct alignment <T[N]> {
	static constexpr size_t value = alignment <T> ::value;
};

template <typename T>
constexpr size_t alignment_v = alignment <T> ::value;
