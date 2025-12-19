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
		// TODO: different for primitives and aggregates...
		return T::reinterpret(jems::array_access(ref, project(idx)));
	}
};
