#pragma once

#include <type_traits>

#include "vector_base.hpp"

template <native_scalar T, size_t N>
struct vector : public vector_base <T, N> {
	using vector_base <T, N> ::vector_base;
	
	using reflection = primitive_reflection <vector <T, N>>;
	DEFINE_REFLECTION_STAMP();
	
	static auto reinterpret(const jems::handle &h) {
		return vector(h);
	}

	// Arithmetic operations
	friend vector operator+(const vector &a, const vector &b) {
		return reinterpret(jems::operation(Operation::eAdd, a, b));
	}

	friend vector operator-(const vector &a, const vector &b) {
		return reinterpret(jems::operation(Operation::eSubtract, a, b));
	}

	friend vector operator-(const vector &v) {
		return reinterpret(jems::operation(Operation::eMultiply, scalar <T> (-1), v));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator+(const U &s, const vector &v) {
		return reinterpret(jems::operation(Operation::eAdd, scalar <T> (s), v));
	}
	
	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator+(const vector &v, const U &s) {
		return reinterpret(jems::operation(Operation::eAdd, v, scalar <T> (s)));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator-(const vector &v, const U &s) {
		return reinterpret(jems::operation(Operation::eSubtract, v, scalar <T> (s)));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator-(const U &s, const vector &v) {
		return reinterpret(jems::operation(Operation::eSubtract, scalar <T> (s), v));
	}
	
	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator*(const U &s, const vector &v) {
		return reinterpret(jems::operation(Operation::eMultiply, scalar <T> (s), v));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator*(const vector &v, const U &s) {
		return reinterpret(jems::operation(Operation::eMultiply, v, scalar <T> (s)));
	}

	friend vector operator/(const vector &a, const vector &b) {
		return reinterpret(jems::operation(Operation::eDivide, a, b));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator/(const vector &v, const U &s) {
		return reinterpret(jems::operation(Operation::eDivide, v, scalar <T> (s)));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator/(const U &s, const vector &v) {
		return reinterpret(jems::operation(Operation::eDivide, scalar <T> (s), v));
	}
};

static_assert(sizeof(vector <int32_t, 2>) == sizeof(jems::handle));
static_assert(sizeof(vector <uint32_t, 3>) == sizeof(jems::handle));
static_assert(sizeof(vector <float, 4>) == sizeof(jems::handle));
