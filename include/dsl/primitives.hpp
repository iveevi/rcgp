#pragma once

#include <cstdlib>
#include <stdfloat>
#include <type_traits>
#include <vector>

#include "jems.hpp"
#include "macro_swizzle.hpp"
#include "../util/logging.hpp"
#include "../meta/reflection.hpp"
#include "../meta/reflection_builder.hpp"

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
class scalar : public jems::handle {
	explicit scalar(const jems::handle &h) : handle(h) {}
public:
	using reflection = primitive_reflection <scalar <T>>;
	DEFINE_REFLECTION_STAMP();

	using native_scalar_type = T;

	scalar() = default;
	
	scalar(const T &value, $location)
		: handle(jems::constant_loc(loc, value)) {}

	static auto reinterpret(const jems::handle &h) {
		return scalar(h);
	}
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
struct vector : public vector_base <T, N> {
	using vector_base <T, N> ::vector_base;
	
	using reflection = primitive_reflection <vector <T, N>>;
	DEFINE_REFLECTION_STAMP();
	
	static auto reinterpret(const jems::handle &h) {
		return vector(h);
	}

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
class matrix : public jems::handle {
	explicit matrix(const jems::handle &h) : handle(h) {}
public:
	using reflection = primitive_reflection <matrix <T, N, M>>;
	DEFINE_REFLECTION_STAMP();

	matrix() = default;

	static auto reinterpret(const jems::handle &h) {
		return matrix(h);
	}
};

using mat4 = matrix <float, 4, 4>;

// concepts to go with the stuff...
template <typename T>
struct is_scalar : std::false_type {};

template <native_scalar T>
struct is_scalar <scalar <T>> : std::true_type {};

template <typename T>
constexpr bool is_scalar_v = is_scalar <T> ::value;

template <typename T>
struct projection {
	using type = T;
};

template <native_scalar T>
struct projection <T> {
	using type = scalar <T>;
};

template <typename T>
using projection_t = projection <T> ::type;

template <typename T>
auto project(const T &value)
{
	return projection_t <T> (value);
}

template <typename T, typename U>
concept projectively_equivalent = std::same_as <
	projection_t <T>,
	projection_t <U>
>;

template <typename T>
concept projectively_scalar = is_scalar_v <projection_t <T>>;

template <typename T>
concept projectively_int_scalar = projectively_scalar <T>
	&& native_int_scalar <
		typename projection_t <T> ::native_scalar_type
	>;

template <reflected T, int64_t N = -1>
struct array : public jems::handle {
	using reflection = array_reflection <T, N>;
	DEFINE_REFLECTION_STAMP();

	template <projectively_int_scalar U>
	T operator[](const U &idx) const {
		// TODO: different for primitives and aggregates...
		return T::reinterpret(jems::array_access(ref, project(idx)));
	}
};

// TODO: capture source location here...
template <typename T>
struct location_proxy {};

// TODO: move to arithmetic
template <native_scalar T, size_t N, size_t M>
vector <T, M> operator*(const matrix <T, N, M> &m, const vector <T, N> &v)
{
	return vector <T, M> ::reinterpret(jems::operation(Operation::eMultiply, m, v));
}

template <native_scalar T, size_t N, size_t M, size_t K>
matrix <T, N, M> operator*(const matrix <T, N, K> &a, const matrix <T, K, M> &b)
{
	return matrix <T, N, M> ::reinterpret(jems::operation(Operation::eMultiply, a, b));
}

// TODO: move to builtin

template <typename T, typename U>
requires projectively_equivalent <T, U>
// TODO: needs to be a scalar? (or vector/matrix)
// projectively_scalar..., projectively_vector <T, D>, ... projectively_vector_dim <D>, _type <T>
auto max(const T &a, const U &b)
{
	using result = projection_t <T>;
	return result::reinterpret(
		jems::builtin_intrinsic(
			BuiltinIntrinsic::eMax,
			project(a), project(b)
		)
	);
}

template <native_scalar T, size_t D>
scalar <T> dot(const vector <T, D> &a, const vector <T, D> &b)
{
	return scalar <T> ::reinterpret(jems::builtin_intrinsic(BuiltinIntrinsic::eDot, a, b));
}

template <native_float_scalar T, size_t N>
vector <T, N> normalize(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(jems::builtin_intrinsic(BuiltinIntrinsic::eNormalize, v));
}
