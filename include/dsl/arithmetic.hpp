#pragma once

#include "vector.hpp"
#include "matrix.hpp"

// TODO: move to arithmetic
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
