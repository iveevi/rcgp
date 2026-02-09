#pragma once

#include "projection.hpp"
#include "vector.hpp"
#include "matrix.hpp"
#include "aliases.hpp"

#define builtin(code, ...)					\
	jems::builtin_intrinsic(BuiltinIntrinsicCode::code,	\
		std::vector <Reference> { __VA_ARGS__ })

namespace rcgp {

template <native_scalar T>
scalar <T> abs(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eAbs, v));
}

template <native_scalar T, size_t N>
vector <T, N> abs(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(builtin(eAbs, v));
}

template <typename T, typename U>
requires projectively_equivalent <T, U>
auto max(const T &a, const U &b)
{
	using result = projection_t <T>;
	return result::reinterpret(
		builtin(eMax, project(a), project(b))
	);
}

template <typename T, typename U>
requires projectively_equivalent <T, U>
auto min(const T &a, const U &b)
{
	using result = projection_t <T>;
	return result::reinterpret(
		builtin(eMin, project(a), project(b))
	);
}

template <typename T, typename U, typename V>
requires projectively_equivalent <T, U> && projectively_equivalent <T, V>
auto clamp(const T &v, const U &lo, const V &hi)
{
	return min(max(v, lo), hi);
}

template <typename T, typename U, typename V>
requires projectively_equivalent <T, U> && projectively_equivalent <T, V>
auto mix(const T &a, const U &b, const V &t)
{
	return a + (b - a) * t;
}

template <typename T, typename U>
requires projectively_equivalent <T, U>
auto pow(const T &a, const U &b)
{
	using result = projection_t <T>;
	return result::reinterpret(
		builtin(ePow, project(a), project(b))
	);
}

inline f32 to_float(const u32 &v)
{
	return f32::reinterpret(builtin(eToFloat, v));
}

inline f32 to_float(const i32 &v)
{
	return f32::reinterpret(builtin(eToFloat, v));
}

template <native_float_scalar T>
scalar <T> sin(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eSin, v));
}

template <native_float_scalar T>
scalar <T> cos(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eCos, v));
}

template <native_float_scalar T>
scalar <T> sqrt(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eSqrt, v));
}

template <native_float_scalar T, size_t N>
vector <T, N> sin(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(builtin(eSin, v));
}

template <native_scalar T, size_t D>
scalar <T> dot(const vector <T, D> &a, const vector <T, D> &b)
{
	return scalar <T> ::reinterpret(builtin(eDot, a, b));
}

template <native_float_scalar T, size_t N>
vector <T, N> normalize(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(builtin(eNormalize, v));
}

template <native_float_scalar T, size_t N>
scalar <T> length(const vector <T, N> &v)
{
	return scalar <T> ::reinterpret(builtin(eLength, v));
}

template <native_float_scalar T>
vector <T, 3> cross(const vector <T, 3> &a, const vector <T, 3> &b)
{
	return vector <T, 3> ::reinterpret(builtin(eCross, a, b));
}

template <native_scalar T, size_t N, size_t M>
matrix <T, M, N> transpose(const matrix <T, N, M> &m)
{
	return matrix <T, M, N> ::reinterpret(builtin(eTranspose, m));
}

template <native_float_scalar T, size_t N>
matrix <T, N, N> inverse(const matrix <T, N, N> &m)
{
	return matrix <T, N, N> ::reinterpret(builtin(eInverse, m));
}

template <native_float_scalar T>
scalar <T> dFdx(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eDFdx, v));
}

template <native_float_scalar T>
scalar <T> dFdy(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eDFdy, v));
}

template <native_float_scalar T>
scalar <T> dFdxFine(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eDFdxFine, v));
}

template <native_float_scalar T>
scalar <T> dFdyFine(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eDFdyFine, v));
}

template <native_float_scalar T, size_t N>
vector <T, N> dFdx(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(builtin(eDFdx, v));
}

template <native_float_scalar T, size_t N>
vector <T, N> dFdy(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(builtin(eDFdy, v));
}

template <native_float_scalar T, size_t N>
vector <T, N> dFdxFine(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(builtin(eDFdxFine, v));
}

template <native_float_scalar T, size_t N>
vector <T, N> dFdyFine(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(builtin(eDFdyFine, v));
}

template <typename T>
T select(const boolean &cond, const T &a, const T &b)
{
	return T::reinterpret(builtin(eSelect, cond, a, b));
}

inline f32 smoothstep(const f32 &x, const f32 &a, const f32 &b)
{
	return f32::reinterpret(builtin(eSmoothstep, x, a, b));
}

} // namespace rcgp

#undef builtin
