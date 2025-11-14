#pragma once

#include <cstdlib>
#include <stdfloat>
#include <type_traits>

#include "jems.hpp"

template <typename T>
struct scalar : jems::handle {
	static_assert(std::is_arithmetic_v <T>);

	scalar() = default;
	scalar(const T &value, $location)
		: handle(jems::constant_loc(loc, value)) {}
};

using i32 = scalar <int32_t>;
using f32 = scalar <float>;

template <typename T, size_t N>
struct vector_base : jems::handle {};

template <typename T>
struct vector_base <T, 3> : jems::handle {
	vector_base() = default;

	vector_base(const jems::handle &h) : handle(h) {}
	
	vector_base(const vector_base <T, 2> &xy, const scalar <T> &z, $location)
		: handle(jems::construct_loc(loc,
			jems::type_loc(loc, VectorType <T, 3> ()),
			xy, z
		)) {}
};

template <typename T>
struct vector_base <T, 4> : jems::handle {
	vector_base() = default;

	vector_base(const jems::handle &h) : handle(h) {}
	
	vector_base(const vector_base <T, 3> &xyz, const scalar <T> &w, $location)
		: handle(jems::construct_loc(loc,
			jems::type_loc(loc, VectorType <T, 4> ()),
			xyz, w
		)) {}

	vector_base(const vector_base <T, 2> &xy, const scalar <T> &z, const scalar <T> &w, $location)
		: handle(jems::construct_loc(loc,
			jems::type_loc(loc, VectorType <T, 4> ()),
			xy, z, w
		)) {}
};

template <typename T, size_t N>
struct vector : vector_base <T, N> {
	static_assert(std::is_arithmetic_v <T>);

	using vector_base <T, N> ::vector_base;

	// Arithmetic operations
	template <typename U>
	requires std::is_convertible_v <U, T>
	friend vector operator*(const U &, const vector &) {}
	
	template <typename U>
	requires std::is_convertible_v <U, T>
	friend vector operator+(const U &, const vector &) {}
};

// TODO: float32_t
using vec2 = vector <float, 2>;
using vec3 = vector <float, 3>;
using vec4 = vector <float, 4>;

using ivec3 = vector <int32_t, 3>;

template <typename T, size_t N, size_t M>
struct matrix : jems::handle {
	static_assert(std::is_arithmetic_v <T>);
};

using mat4 = matrix <float, 4, 4>;

template <typename T>
struct location_proxy {};

template <typename T, size_t N, size_t M>
vector <T, M> operator*(const matrix <T, N, M> &m, const vector <T, N> &v)
{
	return jems::operation(Operation::eMultiply, m, v);
}

template <typename T, size_t N, size_t M, size_t K>
matrix <T, N, M> operator*(const matrix <T, N, K> &, const matrix <T, K, M> &)
{
}

vec3 cross(vec3, vec3) {}
vec3 dFdx(vec3) {}
vec3 dFdy(vec3) {}

template <typename T, size_t N>
vector <T, N> normalize(const vector <T, N> &)
{
}
