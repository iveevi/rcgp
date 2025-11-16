#pragma once

#include "../dsl/jems.hpp"
#include "../dsl/primitives.hpp"
#include "reference.hpp"
#include "reflection.hpp"

// TODO: needs a layout...
template <typename T>
struct ParameterBlock {
	using reflection = parameter_block_reflection <T>;
	UGP_REFLECTION_STAMP;
};

template <typename T>
struct StructuredBuffer : T {
	using reflection = structured_buffer_reflection <T>;
	UGP_REFLECTION_STAMP;
};

template <typename T, size_t D>
struct Sampler : jems::handle {
	using reflection = sampler_reflection <T, D>;
	UGP_REFLECTION_STAMP;

	// TODO: requires floating stuff
	vector <T, D> sample(vector <float, D> x, $location) {
		return  jems::builtin_intrinsic_loc(loc, BuiltinIntrinsic::eSample, ref, x);
	}
};

using Sampler1D = Sampler <float, 1>;
using Sampler2D = Sampler <float, 2>;
using Sampler3D = Sampler <float, 3>;

// TODO: RWStructuredBuffer as an alias with RW flags...

template <typename T>
struct RayPayload : T {};

// Inspection
template <typename T>
struct is_resource_reflection : std::false_type {};

template <typename T>
struct is_resource_reflection <structured_buffer_reflection <T>> : std::true_type {};

template <typename T, size_t D>
struct is_resource_reflection <sampler_reflection <T, D>> : std::true_type {};

template <typename T>
constexpr bool is_resource_reflection_v = is_resource_reflection <T> ::value;

// Overriding reference behavior
template <typename T, ParameterBlock <T> &rsrc>
struct reference_base <rsrc> {
	using type = T;
};
