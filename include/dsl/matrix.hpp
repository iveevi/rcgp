#pragma once

#include <type_traits>

#include "scalar.hpp"
#include "primitive_of.hpp"

namespace rcgp {

template <native_scalar T, size_t N, size_t M>
class matrix : public jems::handle {
	explicit matrix(const jems::handle &h) : handle(h) {}
public:
	matrix() = default;

	template <size_t A, size_t B>
	explicit matrix(const matrix <T, A, B> &other)
		: handle(wrap_in_local(
			std::source_location::current(),
			jems::type(primitive_of <T, N, M> ()),
			{ other }
		)) {}

	static auto reinterpret(const jems::handle &h) {
		auto type = jems::type(primitive_of <T, N, M> ());
		auto local = jems::local(type);
		jems::store(local, h);
		return matrix (local);
	}

	matrix &operator=(const matrix &rhs) {
		if (not _ref)
			_ref = jems::local(jems::type(primitive_of <T, N, M> ()));
		jems::store(_ref, rhs);
		return *this;
	}
	
	// Arithmetic operations
	template <typename U>
	requires std::is_convertible_v <U, scalar <T>>
	friend matrix operator*(const U &s, const matrix &m) {
		return reinterpret(jems::operation(OperationCode::eMultiply, scalar <T> (s), m));
	}

	friend matrix operator-(const matrix &m) {
		return reinterpret(jems::operation(OperationCode::eMultiply, scalar <T> (-1), m));
	}
};

} // namespace rcgp
