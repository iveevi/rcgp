#pragma once

#include <type_traits>

#include "vector_base.hpp"

namespace rcgp {

template <native_scalar T, size_t N>
struct vector : public vector_base <T, N> {
	using vector_base <T, N> ::vector_base;
	
	// TODO: only upcast to local if its being stored into...
	static auto reinterpret(const jems::handle &h) {
		auto type = jems::type(primitive_of <T, N> ());
		auto local = jems::local(type);
		jems::store(local, h);
		return vector(local);
	}

	vector &operator=(const vector &rhs) {
		if (not this->_ref)
			this->_ref = jems::local(jems::type(primitive_of <T, N> ()));
		jems::store(this->_ref, rhs);
		return *this;
	}

	// Arithmetic operations
	// TODO: move to a header/generate with a script?
	friend vector operator+(const vector &a, const vector &b) {
		return reinterpret(jems::operation(OperationCode::eAdd, a, b));
	}
	
	friend vector operator*(const vector &a, const vector &b) {
		return reinterpret(jems::operation(OperationCode::eMultiply, a, b));
	}

	friend vector operator-(const vector &a, const vector &b) {
		return reinterpret(jems::operation(OperationCode::eSubtract, a, b));
	}

	friend vector operator-(const vector &v) {
		// TODO: negative operation
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

} // namespace rcgp
