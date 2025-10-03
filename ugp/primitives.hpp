#pragma once

#include <cstdlib>
#include <stdfloat>
#include <type_traits>

template <typename T>
struct scalar {
	static_assert(std::is_arithmetic_v <T>);
};

using i32 = scalar <int32_t>;

template <typename T, size_t N>
struct vector {
	static_assert(std::is_arithmetic_v <T>);
};

// TODO: float32_t
using vec2 = vector <float, 2>;
using vec3 = vector <float, 3>;
using vec4 = vector <float, 4>;

using ivec3 = vector <int32_t, 3>;

template <typename T, size_t N, size_t M>
struct matrix {
	static_assert(std::is_arithmetic_v <T>);
};

using mat4 = matrix <float, 4, 4>;
