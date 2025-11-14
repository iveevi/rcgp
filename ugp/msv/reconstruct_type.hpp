#pragma once

#include "../dsl/jems.hpp"
#include "../dsl/primitives.hpp"

#include "static_string.hpp"

template <typename T>
auto reconstruct_type()
{
	// TODO: specialization structures to avoid explicit lists like this...
	if constexpr (std::is_same_v <T, i32>)
		return jems::type(int32_t(0));
	else if constexpr (std::is_same_v <T, vec2>)
		return jems::type(VectorType <float, 2> ());
	else if constexpr (std::is_same_v <T, vec3>)
		return jems::type(VectorType <float, 3> ());
	else if constexpr (std::is_same_v <T, mat4>)
		return jems::type(MatrixType <float, 4, 4> ());
	else
		static_assert(false, ($ss("failed to reconstruct type ") + $ss_type(T)).view());
}
