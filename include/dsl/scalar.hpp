#pragma once

#include <source_location>
#include <stdfloat>

#include "../meta/reflection.hpp"
#include "../meta/reflection_builder.hpp"
#include "jems.hpp"
#include "local.hpp"
#include "primitives_concepts.hpp"


// Scalars in GPU code
template <native_scalar T>
class scalar : public jems::handle {
	explicit scalar(const jems::handle &h) : handle(h) {}
public:
	using reflection = primitive_reflection <scalar <T>>;
	DEFINE_REFLECTION_STAMP();

	using native_scalar_type = T;

	scalar() {
		if (!Tracer::singleton.records.empty()) {
			auto type = jems::type_loc(std::source_location::current(), T());
			init_local_if_tracing(*this, type);
		}
	}
	
	scalar(const T &value, $location)
		: handle() {
		if (Tracer::singleton.records.empty()) {
			ref = jems::constant_loc(loc, value);
			return;
		}

		auto local = jems::local(jems::type_loc(loc, T()));
		ref = local;
		jems::store(local, jems::constant_loc(loc, value));
	}

	scalar &operator=(const scalar &rhs) {
		if (Tracer::singleton.records.empty()) {
			ref = rhs.ref;
			return *this;
		}

		auto type = jems::type_loc(std::source_location::current(), T());
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

	static auto reinterpret(const jems::handle &h) {
		return scalar(h);
	}
};

using boolean = scalar <bool>;
using i32 = scalar <int32_t>;
using u32 = scalar <uint32_t>;
using f32 = scalar <float>;

extern template class scalar <int32_t>;
extern template class scalar <uint32_t>;
extern template class scalar <float>;
