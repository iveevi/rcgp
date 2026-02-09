#pragma once

#include <cstdlib>
#include <type_traits>

#include "local.hpp"
#include "primitive_of.hpp"
#include "pygen_macro_swizzle.hpp"
#include "scalar.hpp"

namespace rcgp {

template <SwizzleCode S, typename T, typename R>
struct swizzle_component {
	// TODO: need to think about swizzles later...

	// swizzle_component &operator=(const swizzle_component &other) = delete;

	// auto &operator=(const swizzle_component &other) {
	// 	info("copy assigning swizzle!");
	// 	return *this;
	// }

	operator R() const {
		static_assert(std::is_base_of_v <jems::handle, T>);
		auto &self = *reinterpret_cast <const T *> (this);
		return R::reinterpret(jems::swizzle(S, self));
	}
};

template <native_scalar T, size_t D>
struct vector;

template <native_scalar T, size_t D>
struct vector_base : jems::handle {};

// TODO: lift restriction on tracer being active unless we are storing...
template <native_scalar T>
class vector_base <T, 2> : public jems::handle {
protected:
	explicit vector_base(const jems::handle &h) : handle(h) {}
public:
	SWIZZLE_D2;

	vector_base() {
		// TODO: Tracer is_active()
		if (not Tracer::singleton.records.empty()) {
			auto type = jems::type(primitive_of <T, 2> ());
			init_local_if_tracing(*this, type);
		}
	}
	
	vector_base(const scalar <T> &x, $location)
		: handle(wrap_in_local(loc,
			jems::type(primitive_of <T, 2> (), loc),
			{ x, x }
		)) {}
	
	vector_base(const scalar <T> &x, const scalar <T> &y, $location)
		: handle(wrap_in_local(loc,
			jems::type(primitive_of <T, 2> (), loc),
			{ x, y }
		)) {}
};

template <native_scalar T>
class vector_base <T, 3> : public jems::handle {
protected:
	explicit vector_base(const jems::handle &h) : handle(h) {}
public:
	SWIZZLE_D3;

	vector_base() {
		if (not Tracer::singleton.records.empty()) {
			auto type = jems::type(primitive_of <T, 3> ());
			init_local_if_tracing(*this, type);
		}
	}

	vector_base(const scalar <T> x, $location)
		: handle(wrap_in_local(loc,
			jems::type(primitive_of <T, 3> (), loc),
			{ x, x, x }
		)) {}

	vector_base(const vector_base <T, 4> v, $location)
		: handle(wrap_in_local(loc,
			jems::type(primitive_of <T, 3> (), loc),
			{ v }
		)) {}
	
	vector_base(const vector_base <T, 2> &xy, const scalar <T> &z, $location)
		: handle(wrap_in_local(loc,
			jems::type(primitive_of <T, 3> (), loc),
			{ xy, z }
		)) {}
	
	vector_base(const scalar <T> &x, const scalar <T> &y, const scalar <T> &z, $location)
		: handle(wrap_in_local(loc,
			jems::type(primitive_of <T, 3> (), loc),
			{ x, y, z }
		)) {}
};

template <native_scalar T>
class vector_base <T, 4> : public jems::handle {
protected:
	explicit vector_base(const jems::handle &h) : handle(h) {}
public:
	SWIZZLE_D4;

	vector_base() {
		if (not Tracer::singleton.records.empty()) {
			auto type = jems::type(primitive_of <T, 4> ());
			init_local_if_tracing(*this, type);
		}
	}
	
	vector_base(const scalar <T> &x, $location)
		: handle(wrap_in_local(loc,
			jems::type(primitive_of <T, 4> (), loc),
			{ x, x, x, x }
		)) {}
	
	vector_base(const vector_base <T, 3> &xyz, const scalar <T> &w, $location)
		: handle(wrap_in_local(loc,
			jems::type(primitive_of <T, 4> (), loc),
			{ xyz, w }
		)) {}

	vector_base(const vector_base <T, 2> &xy, const scalar <T> &z, const scalar <T> &w, $location)
		: handle(wrap_in_local(loc,
			jems::type(primitive_of <T, 4> (), loc),
			{ xy, z, w }
		)) {}
	
	vector_base(const vector_base <T, 2> &xy, const vector_base <T, 2> &zw, $location)
		: handle(wrap_in_local(loc,
			jems::type(primitive_of <T, 4> (), loc),
			{ xy, zw }
		)) {}

	vector_base(const scalar <T> &x, const scalar <T> &y, const scalar <T> &z, const scalar <T> &w, $location)
		: handle(wrap_in_local(loc,
			jems::type(primitive_of <T, 4> (), loc),
			{ x, y, z, w }
		)) {}
};

} // namespace rcgp
