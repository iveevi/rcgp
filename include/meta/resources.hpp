#pragma once

#include "../dsl/jems.hpp"
#include "../dsl/primitives.hpp"
#include "reference.hpp"
#include "reflection.hpp"
#include "reflection_builder.hpp"

template <reflected T>
struct ResourceGroup {
	using reflection = resource_group_reflection <T>;
	DEFINE_REFLECTION_STAMP();
};

template <reflected T>
struct ConstantBuffer : T {
	using reflection = constant_buffer_reflection <T>;
	DEFINE_REFLECTION_STAMP();
};

// TODO: Read/write flags and aliases...
template <reflected T>
struct StorageBuffer : T {
	using reflection = storage_buffer_reflection <T>;
	DEFINE_REFLECTION_STAMP();
};

template <reflected T>
using ArrayBuffer = StorageBuffer <array <T>>;

template <native_scalar T, size_t D>
struct Sampler : jems::handle {
	using reflection = sampler_reflection <T, D>;
	DEFINE_REFLECTION_STAMP();

	vector <T, D> sample(vector <T, D> x, $location) const
		requires native_float_scalar <T>
	{
		return  jems::builtin_intrinsic_loc(loc, BuiltinIntrinsic::eSample, ref, x);
	}
};

using Sampler1D = Sampler <float, 1>;
using Sampler2D = Sampler <float, 2>;
using Sampler3D = Sampler <float, 3>;

// NOTE: ray payload will be a resource block!
template <typename T>
struct RayPayload : T {};

// Aliases for single resource blocks
template <reflected T>
using MonoConstantBuffer = ResourceGroup <ConstantBuffer <T>>;

template <reflected T>
using MonoStorageBuffer = ResourceGroup <StorageBuffer <T>>;

// Inspection
template <typename T>
struct is_resource_reflection : std::false_type {};

template <typename T>
struct is_resource_reflection <constant_buffer_reflection <T>> : std::true_type {};

template <typename T>
struct is_resource_reflection <storage_buffer_reflection <T>> : std::true_type {};

template <typename T, size_t D>
struct is_resource_reflection <sampler_reflection <T, D>> : std::true_type {};

template <typename T>
constexpr bool is_resource_reflection_v = is_resource_reflection <T> ::value;

// Overriding reference behavior
template <typename T, ResourceGroup <T> &rsrc>
struct reference_base <rsrc> {
	using type = T;
};
