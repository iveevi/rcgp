#pragma once

#include <stdfloat>

#include "../meta/reflection.hpp"
#include "../meta/reflection_builder.hpp"
#include "jems.hpp"
#include "primitives_concepts.hpp"

// Scalars in GPU code
template <native_scalar T>
class scalar : public jems::handle {
	explicit scalar(const jems::handle &h) : handle(h) {}
public:
	using reflection = primitive_reflection <scalar <T>>;
	DEFINE_REFLECTION_STAMP();

	using native_scalar_type = T;

	scalar() = default;
	
	scalar(const T &value, $location)
		: handle(jems::constant_loc(loc, value)) {}

	friend scalar operator-(const scalar &v) {
		return scalar(jems::operation(Operation::eMultiply, scalar <T> (-1), v));
	}

	static auto reinterpret(const jems::handle &h) {
		return scalar(h);
	}
};

using i32 = scalar <int32_t>;
using u32 = scalar <uint32_t>;
using f32 = scalar <float>;
