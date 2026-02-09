#pragma once

#include <source_location>
#include <stdfloat>

#include "jems.hpp"
#include "local.hpp"
#include "primitive_of.hpp"
#include "primitives_concepts.hpp"

namespace rcgp {

auto wrap_in_local(auto loc, auto type, const std::vector <Reference> &cargs)
{
	auto c = jems::construct(type, cargs, loc);
	auto l = jems::local(type, loc);
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
			auto type = jems::type(primitive_of <T> (), std::source_location::current());
			init_local_if_tracing(*this, type);
		}
	}
	
	scalar(const T &value, $location)
		: handle() {
		if (Tracer::singleton.records.empty()) {
			_ref = jems::constant(value, loc);
			return;
		}

		auto local = jems::local(jems::type(primitive_of <T> (), loc));
		_ref = local;
		jems::store(local, jems::constant(value, loc));
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
		return reinterpret(jems::operation(OperationCode::eMultiply, scalar <T> (-1), v));
	}
	
	friend scalar operator!(const scalar &v)
	requires std::is_same_v <T, bool> {
		return reinterpret(jems::operation(OperationCode::eLogicalNot, v));
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
