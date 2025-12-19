#pragma once

#include "scalar.hpp"

template <native_scalar T, size_t N, size_t M>
class matrix : public jems::handle {
	explicit matrix(const jems::handle &h) : handle(h) {}
public:
	using reflection = primitive_reflection <matrix <T, N, M>>;
	DEFINE_REFLECTION_STAMP();

	matrix() = default;

	template <size_t A, size_t B>
	explicit matrix(const matrix <T, A, B> &other)
		: handle(jems::construct(
			jems::type(MatrixType <T, N, M> ()),
			other
		)) {}

	static auto reinterpret(const jems::handle &h) {
		return matrix(h);
	}
	
	// Arithmetic operations
	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend matrix operator*(const U &s, const matrix &m) {
		return reinterpret(jems::operation(Operation::eMultiply, scalar <T> (s), m));
	}

	friend matrix operator-(const matrix &m) {
		return reinterpret(jems::operation(Operation::eMultiply, scalar <T> (-1), m));
	}
};
