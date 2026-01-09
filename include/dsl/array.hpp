#pragma once

#include "projection.hpp"
#include "../meta/reflection.hpp"
#include "../meta/reflection_builder.hpp"

template <reflected T, int64_t N = -1>
struct array : public jems::handle {
	using reflection = array_reflection <T, N>;
	DEFINE_REFLECTION_STAMP();

	template <projectively_int_scalar U>
	T operator[](const U &idx) const {
		T result;
		auto access = jems::array_access(ref, project(idx));
		inject_reference(result, access);
		return result;
	}
};
