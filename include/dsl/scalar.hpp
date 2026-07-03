#pragma once

#include <stdfloat>

#include "jems.hpp"
#include "primitive_of.hpp"
#include "primitives_concepts.hpp"

namespace rcgp {

// Scalars in GPU code
template <native_scalar T>
class scalar : public jems::handle {
	explicit scalar(const jems::handle &h) : handle(h) {}
public:
	using native_scalar_type = T;

	scalar() = default;

	scalar(const scalar &other, $location) {
		_ref = jems::local(jems::type(primitive_of <T> (), loc));
		jems::store(_ref, other);
	}

	scalar(const T &value, $location) {
		_ref = jems::local(jems::type(primitive_of <T> (), loc));
		jems::store(_ref, jems::constant(value, loc));
	}

	template <native_scalar U>
	requires std::is_convertible_v <U, T>
	scalar(const scalar <U> &value, $location) {
		auto type = jems::type(primitive_of <T> (), loc);
		_ref = jems::local(type, loc);
		if constexpr (std::is_same_v <U, T>)
			jems::store(_ref, value);
		else
			jems::store(_ref, jems::construct(type, { value }, loc));
	}

	scalar &operator=(const scalar &rhs) {
		if (not _ref)
			_ref = jems::local(jems::type(primitive_of <T> ()));
		jems::store(_ref, rhs);
		return *this;
	}

	friend scalar operator+(const scalar &a, const scalar &b) {
		return reinterpret(jems::operation(OperationCode::eAdd, a, b));
	}

	friend scalar operator-(const scalar &a, const scalar &b) {
		return reinterpret(jems::operation(OperationCode::eSubtract, a, b));
	}

	friend scalar operator*(const scalar &a, const scalar &b) {
		return reinterpret(jems::operation(OperationCode::eMultiply, a, b));
	}

	friend scalar operator/(const scalar &a, const scalar &b) {
		return reinterpret(jems::operation(OperationCode::eDivide, a, b));
	}

	friend scalar operator-(const scalar &v) {
		return reinterpret(jems::operation(OperationCode::eMultiply, scalar <T> (-1), v));
	}
	
	friend scalar operator!(const scalar &v)
	requires std::is_same_v <T, bool> {
		return reinterpret(jems::operation(OperationCode::eLogicalNot, v));
	}

	// TODO: shouldnt apply for boolean
	scalar &operator+=(const scalar &other) {
		*this = *this + other;
		return *this;
	}
	
	scalar &operator-=(const scalar &other) {
		*this = *this - other;
		return *this;
	}

	scalar &operator*=(const scalar &other) {
		*this = *this * other;
		return *this;
	}
	
	scalar &operator/=(const scalar &other) {
		*this = *this / other;
		return *this;
	}

	scalar &operator++() {
		*this = *this + scalar <T> (1);
		return *this;
	}

	scalar operator++(int) {
		auto prev = *this;
		++(*this);
		return prev;
	}

	scalar &operator--() {
		*this = *this - scalar <T> (1);
		return *this;
	}

	scalar operator--(int) {
		auto prev = *this;
		--(*this);
		return prev;
	}

	static scalar reinterpret(const jems::handle &h) {
		auto type = jems::type(primitive_of <T> ());
		auto local = jems::local(type);
		jems::store(local, h);
		return scalar(local);
	}
};

} // namespace rcgp
