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
		return reinterpret(jems::operation(OperationCode::eAdd, a, b));
	}

	friend vector operator-(const vector &a, const vector &b) {
		return reinterpret(jems::operation(OperationCode::eSubtract, a, b));
	}

	friend vector operator-(const vector &v) {
		return reinterpret(jems::operation(OperationCode::eMultiply, scalar <T> (-1), v));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator+(const U &s, const vector &v) {
		return reinterpret(jems::operation(OperationCode::eAdd, scalar <T> (s), v));
	}
	
	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator+(const vector &v, const U &s) {
		return reinterpret(jems::operation(OperationCode::eAdd, v, scalar <T> (s)));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator-(const vector &v, const U &s) {
		return reinterpret(jems::operation(OperationCode::eSubtract, v, scalar <T> (s)));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator-(const U &s, const vector &v) {
		return reinterpret(jems::operation(OperationCode::eSubtract, scalar <T> (s), v));
	}
	
	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator*(const U &s, const vector &v) {
		return reinterpret(jems::operation(OperationCode::eMultiply, scalar <T> (s), v));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator*(const vector &v, const U &s) {
		return reinterpret(jems::operation(OperationCode::eMultiply, v, scalar <T> (s)));
	}

	friend vector operator/(const vector &a, const vector &b) {
		return reinterpret(jems::operation(OperationCode::eDivide, a, b));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator/(const vector &v, const U &s) {
		return reinterpret(jems::operation(OperationCode::eDivide, v, scalar <T> (s)));
	}

	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend vector operator/(const U &s, const vector &v) {
		return reinterpret(jems::operation(OperationCode::eDivide, scalar <T> (s), v));
	}
};

static_assert(sizeof(vector <int32_t, 2>) == sizeof(jems::handle));
static_assert(sizeof(vector <uint32_t, 3>) == sizeof(jems::handle));
static_assert(sizeof(vector <float, 4>) == sizeof(jems::handle));

extern template struct vector <int32_t, 2>;
extern template struct vector <int32_t, 3>;
extern template struct vector <int32_t, 4>;
extern template struct vector <uint32_t, 2>;
extern template struct vector <uint32_t, 3>;
extern template struct vector <uint32_t, 4>;
extern template struct vector <float, 2>;
extern template struct vector <float, 3>;
extern template struct vector <float, 4>;
