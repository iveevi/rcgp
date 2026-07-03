#pragma once

#include "canonical.hpp"
#include "vector.hpp"
#include "matrix.hpp"
#include "aliases.hpp"

#define builtin(code, ...)					\
	jems::builtin_intrinsic(BuiltinIntrinsicCode::code,	\
		std::vector <Reference> { __VA_ARGS__ })

namespace rcgp {

template <native_scalar T>
scalar <T> abs(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eAbs, v));
}

template <native_scalar T, size_t N>
vector <T, N> abs(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(builtin(eAbs, v));
}

template <typename T, typename U>
requires canonically_equivalent <T, U>
auto max(const T &a, const U &b)
{
	using result = canonical_type_t <T>;
	return result::reinterpret(
		builtin(eMax, canonicalize(a), canonicalize(b))
	);
}

template <typename T, typename U>
requires canonically_equivalent <T, U>
auto min(const T &a, const U &b)
{
	using result = canonical_type_t <T>;
	return result::reinterpret(
		builtin(eMin, canonicalize(a), canonicalize(b))
	);
}

template <typename T, typename U, typename V>
requires canonically_equivalent <T, U> && canonically_equivalent <T, V>
auto clamp(const T &v, const U &lo, const V &hi)
{
	return min(max(v, lo), hi);
}

template <typename T, typename U, typename V>
requires canonically_equivalent <T, U> && canonically_equivalent <T, V>
auto mix(const T &a, const U &b, const V &t)
{
	return a + (b - a) * t;
}

template <typename T, typename U>
requires canonically_equivalent <T, U>
auto pow(const T &a, const U &b)
{
	using result = canonical_type_t <T>;
	return result::reinterpret(
		builtin(ePow, canonicalize(a), canonicalize(b))
	);
}

// Bitwise reinterpret casts
template <typename To>
struct bitcast_impl;

template <>
struct bitcast_impl <f32> {
	static f32 from(const u32 &v) { return f32::reinterpret(builtin(eUintBitsToFloat, v)); }
	static f32 from(const i32 &v) { return f32::reinterpret(builtin(eIntBitsToFloat, v)); }
};

template <>
struct bitcast_impl <i32> {
	static i32 from(const f32 &v) { return i32::reinterpret(builtin(eFloatBitsToInt, v)); }
};

template <>
struct bitcast_impl <u32> {
	static u32 from(const f32 &v) { return u32::reinterpret(builtin(eFloatBitsToUint, v)); }
};

template <typename To, typename From>
To bitcast(const From &v)
{
	return bitcast_impl <To> ::from(v);
}

inline boolean isnan(const f32 &v)
{
	return boolean::reinterpret(builtin(eIsNan, v));
}

inline boolean isinf(const f32 &v)
{
	return boolean::reinterpret(builtin(eIsInf, v));
}

template <native_float_scalar T>
scalar <T> sin(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eSin, v));
}

template <native_float_scalar T>
scalar <T> cos(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eCos, v));
}

template <native_float_scalar T>
scalar <T> sqrt(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eSqrt, v));
}

template <native_float_scalar T, size_t N>
vector <T, N> sin(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(builtin(eSin, v));
}

template <native_scalar T, size_t D>
scalar <T> dot(const vector <T, D> &a, const vector <T, D> &b)
{
	return scalar <T> ::reinterpret(builtin(eDot, a, b));
}

template <native_float_scalar T, size_t N>
vector <T, N> normalize(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(builtin(eNormalize, v));
}

template <native_float_scalar T, size_t N>
scalar <T> length(const vector <T, N> &v)
{
	return scalar <T> ::reinterpret(builtin(eLength, v));
}

template <native_float_scalar T>
vector <T, 3> cross(const vector <T, 3> &a, const vector <T, 3> &b)
{
	return vector <T, 3> ::reinterpret(builtin(eCross, a, b));
}

template <native_scalar T, size_t N, size_t M>
matrix <T, M, N> transpose(const matrix <T, N, M> &m)
{
	return matrix <T, M, N> ::reinterpret(builtin(eTranspose, m));
}

template <native_float_scalar T, size_t N>
matrix <T, N, N> inverse(const matrix <T, N, N> &m)
{
	return matrix <T, N, N> ::reinterpret(builtin(eInverse, m));
}

template <native_float_scalar T>
scalar <T> dFdx(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eDFdx, v));
}

template <native_float_scalar T>
scalar <T> dFdy(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eDFdy, v));
}

template <native_float_scalar T>
scalar <T> dFdxFine(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eDFdxFine, v));
}

template <native_float_scalar T>
scalar <T> dFdyFine(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eDFdyFine, v));
}

template <native_float_scalar T, size_t N>
vector <T, N> dFdx(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(builtin(eDFdx, v));
}

template <native_float_scalar T, size_t N>
vector <T, N> dFdy(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(builtin(eDFdy, v));
}

template <native_float_scalar T, size_t N>
vector <T, N> dFdxFine(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(builtin(eDFdxFine, v));
}

template <native_float_scalar T, size_t N>
vector <T, N> dFdyFine(const vector <T, N> &v)
{
	return vector <T, N> ::reinterpret(builtin(eDFdyFine, v));
}

template <typename T>
T select(const boolean &cond, const T &a, const T &b)
{
	return T::reinterpret(builtin(eSelect, cond, a, b));
}

inline f32 smoothstep(const f32 &x, const f32 &a, const f32 &b)
{
	return f32::reinterpret(builtin(eSmoothstep, x, a, b));
}

inline u32 nonuniform(const u32 &v)
{
	return u32::reinterpret(builtin(eNonUniformEXT, v));
}

// tan (enum existed but wrapper was missing)
template <native_float_scalar T>
scalar <T> tan(const scalar <T> &v)
{
	return scalar <T> ::reinterpret(builtin(eTan, v));
}

// Rounding
template <native_float_scalar T>
scalar <T> floor(const scalar <T> &v) { return scalar <T> ::reinterpret(builtin(eFloor, v)); }

template <native_float_scalar T, size_t N>
vector <T, N> floor(const vector <T, N> &v) { return vector <T, N> ::reinterpret(builtin(eFloor, v)); }

template <native_float_scalar T>
scalar <T> ceil(const scalar <T> &v) { return scalar <T> ::reinterpret(builtin(eCeil, v)); }

template <native_float_scalar T, size_t N>
vector <T, N> ceil(const vector <T, N> &v) { return vector <T, N> ::reinterpret(builtin(eCeil, v)); }

template <native_float_scalar T>
scalar <T> round(const scalar <T> &v) { return scalar <T> ::reinterpret(builtin(eRound, v)); }

template <native_float_scalar T, size_t N>
vector <T, N> round(const vector <T, N> &v) { return vector <T, N> ::reinterpret(builtin(eRound, v)); }

template <native_float_scalar T>
scalar <T> trunc(const scalar <T> &v) { return scalar <T> ::reinterpret(builtin(eTrunc, v)); }

template <native_float_scalar T, size_t N>
vector <T, N> trunc(const vector <T, N> &v) { return vector <T, N> ::reinterpret(builtin(eTrunc, v)); }

template <native_float_scalar T>
scalar <T> fract(const scalar <T> &v) { return scalar <T> ::reinterpret(builtin(eFract, v)); }

template <native_float_scalar T, size_t N>
vector <T, N> fract(const vector <T, N> &v) { return vector <T, N> ::reinterpret(builtin(eFract, v)); }

template <native_float_scalar T>
scalar <T> sign(const scalar <T> &v) { return scalar <T> ::reinterpret(builtin(eSign, v)); }

template <native_float_scalar T, size_t N>
vector <T, N> sign(const vector <T, N> &v) { return vector <T, N> ::reinterpret(builtin(eSign, v)); }

// Modulo and step
template <typename T, typename U>
requires canonically_equivalent <T, U>
auto mod(const T &a, const U &b)
{
	using result = canonical_type_t <T>;
	return result::reinterpret(builtin(eMod, canonicalize(a), canonicalize(b)));
}

template <typename T, typename U>
requires canonically_equivalent <T, U>
auto step(const T &edge, const U &x)
{
	using result = canonical_type_t <T>;
	return result::reinterpret(builtin(eStep, canonicalize(edge), canonicalize(x)));
}

// Exponential
template <native_float_scalar T>
scalar <T> exp(const scalar <T> &v) { return scalar <T> ::reinterpret(builtin(eExp, v)); }

template <native_float_scalar T, size_t N>
vector <T, N> exp(const vector <T, N> &v) { return vector <T, N> ::reinterpret(builtin(eExp, v)); }

template <native_float_scalar T>
scalar <T> exp2(const scalar <T> &v) { return scalar <T> ::reinterpret(builtin(eExp2, v)); }

template <native_float_scalar T, size_t N>
vector <T, N> exp2(const vector <T, N> &v) { return vector <T, N> ::reinterpret(builtin(eExp2, v)); }

template <native_float_scalar T>
scalar <T> log(const scalar <T> &v) { return scalar <T> ::reinterpret(builtin(eLog, v)); }

template <native_float_scalar T, size_t N>
vector <T, N> log(const vector <T, N> &v) { return vector <T, N> ::reinterpret(builtin(eLog, v)); }

template <native_float_scalar T>
scalar <T> log2(const scalar <T> &v) { return scalar <T> ::reinterpret(builtin(eLog2, v)); }

template <native_float_scalar T, size_t N>
vector <T, N> log2(const vector <T, N> &v) { return vector <T, N> ::reinterpret(builtin(eLog2, v)); }

// Trigonometric
template <native_float_scalar T>
scalar <T> asin(const scalar <T> &v) { return scalar <T> ::reinterpret(builtin(eAsin, v)); }

template <native_float_scalar T>
scalar <T> acos(const scalar <T> &v) { return scalar <T> ::reinterpret(builtin(eAcos, v)); }

template <native_float_scalar T>
scalar <T> atan(const scalar <T> &v) { return scalar <T> ::reinterpret(builtin(eAtan, v)); }

template <typename T, typename U>
requires canonically_equivalent <T, U>
auto atan(const T &y, const U &x)
{
	using result = canonical_type_t <T>;
	return result::reinterpret(builtin(eAtan, canonicalize(y), canonicalize(x)));
}

template <native_float_scalar T>
scalar <T> sinh(const scalar <T> &v) { return scalar <T> ::reinterpret(builtin(eSinh, v)); }

template <native_float_scalar T>
scalar <T> cosh(const scalar <T> &v) { return scalar <T> ::reinterpret(builtin(eCosh, v)); }

template <native_float_scalar T>
scalar <T> tanh(const scalar <T> &v) { return scalar <T> ::reinterpret(builtin(eTanh, v)); }

// Geometry
template <native_float_scalar T, size_t N>
vector <T, N> reflect(const vector <T, N> &I, const vector <T, N> &n)
{
	return vector <T, N> ::reinterpret(builtin(eReflect, I, n));
}

template <native_float_scalar T, size_t N>
vector <T, N> refract(const vector <T, N> &I, const vector <T, N> &n, const scalar <T> &eta)
{
	return vector <T, N> ::reinterpret(builtin(eRefract, I, n, eta));
}

template <native_float_scalar T, size_t N>
scalar <T> distance(const vector <T, N> &a, const vector <T, N> &b)
{
	return scalar <T> ::reinterpret(builtin(eDistance, a, b));
}

template <typename T, typename U, typename V>
requires canonically_equivalent <T, U> && canonically_equivalent <T, V>
auto fma(const T &a, const U &b, const V &c)
{
	using result = canonical_type_t <T>;
	return result::reinterpret(builtin(eFma, canonicalize(a), canonicalize(b), canonicalize(c)));
}

} // namespace rcgp

#undef builtin
