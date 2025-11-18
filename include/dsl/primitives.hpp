#pragma once

#include <cstdlib>
#include <stdfloat>
#include <type_traits>

#include "jems.hpp"
#include "macro_swizzle.hpp"
#include "../util/logging.hpp"
#include "../msv/reflection.hpp"

// Classifying host primitive types
template <typename T>
concept native_scalar = bool(false
	| std::same_as <T, bool>
	| std::same_as <T, uint32_t>
	| std::same_as <T, int32_t>
	| std::same_as <T, float>
	| std::same_as <T, uint64_t>);

template <typename T>
concept native_int_scalar = bool(false
	| std::same_as <T, uint32_t>
	| std::same_as <T, int32_t>);

template <typename T>
concept native_float_scalar = bool(false
	| std::same_as <T, float>);

// Scalars in GPU code
template <native_scalar T>
struct scalar : jems::handle {
	using reflection = primitive_reflection <scalar <T>>;
	UGP_REFLECTION_STAMP;

	scalar() = default;

	scalar(const jems::handle &h) : handle(h) {}
	
	scalar(const T &value, $location)
		: handle(jems::constant_loc(loc, value)) {}
};

using i32 = scalar <int32_t>;
using u32 = scalar <uint32_t>;
using f32 = scalar <float>;

// Vectors in GPU code
template <Swizzle::Code S, typename T, typename R>
struct swizzle_component {
	operator R() const {
		static_assert(std::is_base_of_v <jems::handle, T>);
		auto x = reinterpret_cast <const T *> (this);
		auto c = jems::swizzle(S, x->ref);
		return R(x->ref);
	}

	// TODO: assign operator, ect...
};

template <native_scalar T, size_t D>
struct vector;

template <native_scalar T, size_t D>
struct vector_base : jems::handle {};

template <native_scalar T>
struct vector_base <T, 2> : jems::handle {
	SWIZZLE_D2;

	vector_base() = default;

	vector_base(const jems::handle &h) : handle(h) {}
	
	vector_base(const scalar <T> &x, const scalar <T> &y, $location)
		: handle(jems::construct_loc(loc,
			jems::type_loc(loc, VectorType <T, 2> ()),
			x, y
		)) {}
};

template <native_scalar T>
struct vector_base <T, 3> : jems::handle {
	SWIZZLE_D3;

	vector_base() = default;

	vector_base(const jems::handle &h) : handle(h) {}
	
	vector_base(const vector_base <T, 2> &xy, const scalar <T> &z, $location)
		: handle(jems::construct_loc(loc,
			jems::type_loc(loc, VectorType <T, 3> ()),
			xy, z
		)) {}
};

template <native_scalar T>
struct vector_base <T, 4> : jems::handle {
	SWIZZLE_D4;

	vector_base() = default;

	vector_base(const jems::handle &h) : handle(h) {}
	
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

template <native_scalar T, size_t N>
struct vector : vector_base <T, N> {
	using vector_base <T, N> ::vector_base;
	
	using reflection = primitive_reflection <vector <T, N>>;
	UGP_REFLECTION_STAMP;

	// Arithmetic operations
	template <typename U>
	requires std::is_convertible_v <U, T>
	friend vector operator*(const U &, const vector &) {}
	
	template <typename U>
	requires std::is_convertible_v <U, T>
	friend vector operator+(const U &, const vector &) {}
};

static_assert(sizeof(vector <int32_t, 2>) == sizeof(jems::handle));
static_assert(sizeof(vector <uint32_t, 3>) == sizeof(jems::handle));
static_assert(sizeof(vector <float, 4>) == sizeof(jems::handle));

// TODO: move to aliases.hpp
using vec2 = vector <float, 2>;
using vec3 = vector <float, 3>;
using vec4 = vector <float, 4>;

using ivec3 = vector <int32_t, 3>;

using float2 = vector <float, 2>;
using float3 = vector <float, 3>;
using float4 = vector <float, 4>;

using int2 = vector <int32_t, 2>;
using int3 = vector <int32_t, 3>;
using int4 = vector <int32_t, 4>;

template <native_scalar T, size_t N, size_t M>
struct matrix : jems::handle {
	using reflection = primitive_reflection <matrix <T, N, M>>;
	UGP_REFLECTION_STAMP;

	matrix() = default;

	matrix(const jems::handle &h) : handle(h) {}
};

using mat4 = matrix <float, 4, 4>;

template <reflected T, int64_t N = -1>
struct array : jems::handle {
	using reflection = array_reflection <T, N>;
	UGP_REFLECTION_STAMP;

	template <native_int_scalar U>
	T operator[](const scalar <U> &idx) const {
		return T(jems::array_access(ref, idx.ref));
	}

	template <native_int_scalar U>
	T operator[](U idx) const {
		return (*this)[scalar <U> (idx)];
	}
};

// TODO: capture source location here...
template <typename T>
struct location_proxy {};

template <typename T, size_t N, size_t M>
vector <T, M> operator*(const matrix <T, N, M> &m, const vector <T, N> &v)
{
	return jems::operation(Operation::eMultiply, m, v);
}

template <typename T, size_t N, size_t M, size_t K>
matrix <T, N, M> operator*(const matrix <T, N, K> &a, const matrix <T, K, M> &b)
{
	return jems::operation(Operation::eMultiply, a, b);
}

template <typename T, size_t D>
scalar <T> dot(const vector <T, D> &a, const vector <T, D> &b)
{
	return jems::builtin_intrinsic(BuiltinIntrinsic::eDot, a, b);
}

template <typename T, size_t N>
vector <T, N> normalize(const vector <T, N> &)
{
}
