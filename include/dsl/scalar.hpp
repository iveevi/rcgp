#pragma once

#include <source_location>
#include <stdfloat>

#include "jems.hpp"
#include "local.hpp"
#include "primitive_of.hpp"
#include "primitives_concepts.hpp"

namespace rcgp {

auto wrap_in_local(auto loc, auto type, auto ... cargs)
{
	auto c = jems::construct_loc(loc, type, cargs...);
	auto l = jems::local_loc(loc, type);
	jems::store(l, c);
	return l;
}

// Scalars in GPU code
template <native_scalar T>
class scalar : public jems::handle {
	explicit scalar(const jems::handle &h) : handle(h) {}
public:
	using native_scalar_type = T;

	scalar() {
		if (!Tracer::singleton.records.empty()) {
			auto type = jems::type_loc(std::source_location::current(), primitive_of <T> ());
			init_local_if_tracing(*this, type);
		}
	}
	
	scalar(const T &value, $location)
		: handle() {
		if (Tracer::singleton.records.empty()) {
			_ref = jems::constant_loc(loc, value);
			return;
		}

		auto local = jems::local(jems::type_loc(loc, primitive_of <T> ()));
		_ref = local;
		jems::store(local, jems::constant_loc(loc, value));
	}

	scalar &operator=(const scalar &rhs) {
		if (Tracer::singleton.records.empty()) {
			_ref = rhs._ref;
			return *this;
		}

		auto type = jems::type(primitive_of <T> ());
		assign_or_store(*this, rhs, type);
		return *this;
	}

	friend scalar operator+(const scalar &a, const scalar &b) {
		return scalar(jems::operation(OperationCode::eAdd, a, b));
	}

	friend scalar operator-(const scalar &a, const scalar &b) {
		return scalar(jems::operation(OperationCode::eSubtract, a, b));
	}

	friend scalar operator*(const scalar &a, const scalar &b) {
		return scalar(jems::operation(OperationCode::eMultiply, a, b));
	}

	friend scalar operator/(const scalar &a, const scalar &b) {
		return scalar(jems::operation(OperationCode::eDivide, a, b));
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

	friend scalar operator-(const scalar &v) {
		return scalar(jems::operation(OperationCode::eMultiply, scalar <T> (-1), v));
	}
	
	friend scalar operator!(const scalar &v)
	requires std::is_same_v <T, bool> {
		return scalar(jems::operation(OperationCode::eLogicalNot, v));
	}

	static auto reinterpret(const jems::handle &h) {
		auto type = jems::type(primitive_of <T> ());
		auto local = jems::local(type);
		jems::store(local, h);
		return scalar(local);
	}
};

extern template class scalar <int32_t>;
extern template class scalar <uint32_t>;
extern template class scalar <float>;

} // namespace rcgp
