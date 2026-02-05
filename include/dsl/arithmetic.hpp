#pragma once

#include "vector.hpp"
#include "matrix.hpp"

namespace rcgp {

inline scalar <float> operator+(const scalar <float> &a, const scalar <int32_t> &b)
{
	return scalar <float> ::reinterpret(jems::operation(OperationCode::eAdd, a, b));
}

inline scalar <float> operator-(const scalar <float> &a, const scalar <int32_t> &b)
{
	return scalar <float> ::reinterpret(jems::operation(OperationCode::eSubtract, a, b));
}

inline scalar <int32_t> operator%(const scalar <int32_t> &a, const scalar <int32_t> &b)
{
	return scalar <int32_t> ::reinterpret(jems::operation(OperationCode::eMod, a, b));
}

template <native_scalar T, size_t N, size_t M>
vector <T, M> operator*(const matrix <T, N, M> &m, const vector <T, N> &v)
{
	return vector <T, M> ::reinterpret(jems::operation(OperationCode::eMultiply, m, v));
}

template <native_scalar T, size_t N, size_t M, size_t K>
matrix <T, N, M> operator*(const matrix <T, N, K> &a, const matrix <T, K, M> &b)
{
	return matrix <T, N, M> ::reinterpret(jems::operation(OperationCode::eMultiply, a, b));
}

} // namespace rcgp

#include "pygen_arithmetic_instantiations.hpp"
