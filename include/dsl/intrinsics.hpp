#pragma once

#include "projection.hpp"
#include "vector.hpp"
#include "matrix.hpp"

// TODO: move to builtin

template <typename T, typename U>
requires projectively_equivalent <T, U>
// TODO: needs to be a scalar? (or vector/matrix)
// projectively_scalar..., projectively_vector <T, D>, ... projectively_vector_dim <D>, _type <T>
auto max(const T &a, const U &b)
{
	using result = projection_t <T>;
	return result::reinterpret(
		jems::builtin_intrinsic(
			BuiltinIntrinsicCode::eMax,
			project(a), project(b)
		)
	);
}

template <native_scalar T, size_t D>
scalar <T> dot(const vector <T, D> &a, const vector <T, D> &b)
{
	return scalar <T> ::reinterpret(jems::builtin_intrinsic(BuiltinIntrinsicCode::eDot, a, b));
}

template <native_float_scalar T, size_t N>
vector <T, N> normalize(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(jems::builtin_intrinsic(BuiltinIntrinsicCode::eNormalize, v));
}

template <native_float_scalar T>
vector <T, 3> cross(const vector <T, 3> &a, const vector <T, 3> &b)
{
	return vector <T, 3> ::reinterpret(jems::builtin_intrinsic(BuiltinIntrinsicCode::eCross, a, b));
}

template <native_scalar T, size_t N, size_t M>
matrix <T, M, N> transpose(const matrix <T, N, M> &m)
{
	return matrix <T, M, N> ::reinterpret(jems::builtin_intrinsic(BuiltinIntrinsicCode::eTranspose, m));
}

template <native_float_scalar T, size_t N>
matrix <T, N, N> inverse(const matrix <T, N, N> &m)
{
	return matrix <T, N, N> ::reinterpret(jems::builtin_intrinsic(BuiltinIntrinsicCode::eInverse, m));
}

template <native_float_scalar T>
scalar <T> dFdx(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(jems::builtin_intrinsic(BuiltinIntrinsicCode::eDFdx, v));
}

template <native_float_scalar T>
scalar <T> dFdy(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(jems::builtin_intrinsic(BuiltinIntrinsicCode::eDFdy, v));
}

template <native_float_scalar T>
scalar <T> dFdxFine(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(jems::builtin_intrinsic(BuiltinIntrinsicCode::eDFdxFine, v));
}

template <native_float_scalar T>
scalar <T> dFdyFine(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(jems::builtin_intrinsic(BuiltinIntrinsicCode::eDFdyFine, v));
}

template <native_float_scalar T, size_t N>
vector <T, N> dFdx(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(jems::builtin_intrinsic(BuiltinIntrinsicCode::eDFdx, v));
}

template <native_float_scalar T, size_t N>
vector <T, N> dFdy(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(jems::builtin_intrinsic(BuiltinIntrinsicCode::eDFdy, v));
}

template <native_float_scalar T, size_t N>
vector <T, N> dFdxFine(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(jems::builtin_intrinsic(BuiltinIntrinsicCode::eDFdxFine, v));
}

template <native_float_scalar T, size_t N>
vector <T, N> dFdyFine(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(jems::builtin_intrinsic(BuiltinIntrinsicCode::eDFdyFine, v));
}
