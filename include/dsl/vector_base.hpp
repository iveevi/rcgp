#pragma once

#include <cstdlib>
#include <type_traits>

#include "macro_swizzle.hpp"
#include "scalar.hpp"

template <SwizzleCode S, typename T, typename R>
struct swizzle_component {
	operator R() const {
		static_assert(std::is_base_of_v <jems::handle, T>);
		auto x = reinterpret_cast <const T *> (this);
		auto c = jems::swizzle(S, *x);
		return R::reinterpret(c);
	}

	// TODO: assign operator, ect...
};

template <native_scalar T, size_t D>
struct vector;

template <native_scalar T, size_t D>
struct vector_base : jems::handle {};

template <native_scalar T>
class vector_base <T, 2> : public jems::handle {
protected:
	explicit vector_base(const jems::handle &h) : handle(h) {}
public:
	SWIZZLE_D2;

	vector_base() = default;
	
	vector_base(const scalar <T> &x, const scalar <T> &y, $location)
		: handle(jems::construct_loc(loc,
			jems::type_loc(loc, VectorType <T, 2> ()),
			x, y
		)) {}
};

template <native_scalar T>
class vector_base <T, 3> : public jems::handle {
protected:
	explicit vector_base(const jems::handle &h) : handle(h) {}
public:
	SWIZZLE_D3;

	vector_base() = default;

	vector_base(const scalar <T> x, $location)
		: handle(jems::construct_loc(loc,
			jems::type_loc(loc, VectorType <T, 3> ()),
			x, x, x
		)) {}

	vector_base(const vector_base <T, 4> v, $location)
		: handle(jems::construct_loc(loc,
			jems::type_loc(loc, VectorType <T, 3> ()),
			v
		)) {}
	
	vector_base(const vector_base <T, 2> &xy, const scalar <T> &z, $location)
		: handle(jems::construct_loc(loc,
			jems::type_loc(loc, VectorType <T, 3> ()),
			xy, z
		)) {}
	
	vector_base(const scalar <T> &x, const scalar <T> &y, const scalar <T> &z, $location)
		: handle(jems::construct_loc(loc,
			jems::type_loc(loc, VectorType <T, 3> ()),
			x, y, z
		)) {}
};

template <native_scalar T>
class vector_base <T, 4> : public jems::handle {
protected:
	explicit vector_base(const jems::handle &h) : handle(h) {}
public:
	SWIZZLE_D4;

	vector_base() = default;
	
	vector_base(const scalar <T> &x, $location)
		: handle(jems::construct_loc(loc,
			jems::type_loc(loc, VectorType <T, 4> ()),
			x, x, x, x
		)) {}
	
	vector_base(const vector_base <T, 3> &xyz, const scalar <T> &w, $location)
		: handle(jems::construct_loc(loc,
			jems::type_loc(loc, VectorType <T, 4> ()),
			xyz, w
		)) {}

	vector_base(const vector_base <T, 2> &xy, const scalar <T> &z, const scalar <T> &w, $location)
		: handle(jems::construct_loc(loc,
			jems::type_loc(loc, VectorType <T, 4> ()),
			xy, z, w
		)) {}
};
