#pragma once

#include "intrinsics_common.hpp"
#include "reconstruct_type.hpp"

namespace rcgp {

using InstanceIndex = read_only_intrinsic <SystemValue::eInstanceIndex, ShaderStage::eVertex, i32>;
using VertexIndex = read_only_intrinsic <SystemValue::eVertexIndex, ShaderStage::eVertex, i32>;
using ClipPosition = write_only_intrinsic <SystemValue::eClipPosition, ShaderStage::eVertex, vec4>;

// Interpolation qualifiers for varying attributes
template <builtin T, RateProperties P>
struct Interpolant : jems::handle {
	Interpolant() = default;

	template <typename U>
	requires std::is_convertible_v <U, T>
	Interpolant(const U &value, $location)
		: handle(jems::stage_output(
			reconstruct_type <T> (), 0, P, loc
		))
	{
		jems::store(*this, T(value), loc);
	}
};

// Smooth interpolation
template <builtin T>
struct Smooth : Interpolant <T, RateProperties::eSmooth> {
	static constexpr RateProperties rate = RateProperties::eSmooth;
	using Interpolant <T, RateProperties::eSmooth> ::Interpolant;
};

template <builtin T>
Smooth(const T &) -> Smooth <T>;

// Flat (no) interpolation
template <builtin T>
struct Flat : Interpolant <T, RateProperties::eFlat> {
	static constexpr RateProperties rate = RateProperties::eFlat;
	using Interpolant <T, RateProperties::eFlat> ::Interpolant;
};

template <builtin T>
Flat(const T &) -> Flat <T>;

// Smooth (without perspective correction) interpolation
template <builtin T>
struct NoPerspective : Interpolant <T, RateProperties::eNoPerspective> {
	static constexpr RateProperties rate = RateProperties::eNoPerspective;
	using Interpolant <T, RateProperties::eNoPerspective> ::Interpolant;
};

template <builtin T>
NoPerspective(const T &) -> NoPerspective <T>;

} // namespace rcgp
