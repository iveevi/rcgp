#pragma once

#include "../dsl/jems.hpp"
#include "../dsl/primitives.hpp"
#include "reflection.hpp"
#include "reflection_builder.hpp"
#include "layout/all.hpp"

// Corresponds to vertex buffes
// TODO: shoudl be more restrictive than reflected... at least must be static
template <reflected T,  template <typename> typename L = layouts::scalar, vk::VertexInputRate R = vk::VertexInputRate::eVertex>
struct AttributeStream : T {
	using reflection = attribute_stream_reflection <T, R>;
	DEFINE_REFLECTION_STAMP();
};

// TODO: should not allow index/vertex buffers here
template <reflected T>
struct ResourceGroup : T {
	using reflection = resource_group_reflection <T>;
	DEFINE_REFLECTION_STAMP();
};

template <reflected T>
struct PushConstant : T {
	using reflection = push_constant_reflection <T>;
	DEFINE_REFLECTION_STAMP();
};

// TODO: these need layouts...
template <reflected T, template <typename> typename L = layouts::std430>
struct UniformBuffer : T {
	using reflection = uniform_buffer_reflection <T, L>;
	DEFINE_REFLECTION_STAMP();
};

// TODO: Read/write flags and aliases...
template <reflected T, template <typename> typename L = layouts::std430>
struct StorageBuffer : T {
	using reflection = storage_buffer_reflection <T, L>;
	DEFINE_REFLECTION_STAMP();
};

template <reflected T, template <typename> typename L = layouts::std430>
using ArrayBuffer = StorageBuffer <array <T>, L>;

template <reflected T, template <typename> typename L = layouts::std430>
struct BufferReference {
	// TODO: jems handle corresponding to
	// device_address := uint64_t underneath
};

template <native_scalar T, size_t D>
struct Sampler : jems::handle {
	using reflection = sampler_reflection <T, D>;
	DEFINE_REFLECTION_STAMP();

	auto sample(vector <T, D> x, $location) const
		requires native_float_scalar <T>
	{
		return vector <T, 4> ::reinterpret(jems::builtin_intrinsic_loc(loc, BuiltinIntrinsic::eSample, *this, x));
	}
};

using Sampler1D = Sampler <float, 1>;
using Sampler2D = Sampler <float, 2>;
using Sampler3D = Sampler <float, 3>;

template <typename T>
struct is_sampler : std::false_type {};

template <typename T, size_t D>
struct is_sampler <Sampler <T, D>> : std::true_type {};

template <typename T>
constexpr bool is_sampler_v = is_sampler <T> ::value;

template <reflected T>
struct RayPayload : T {};

// Introspection
// TODO: we can move this to reflection.hpp
template <typename T>
struct is_global_resource_reflection : std::false_type {};

template <typename T>
struct is_global_resource_reflection <resource_group_reflection <T>> : std::true_type {};

template <typename T, template <typename> typename L>
struct is_global_resource_reflection <uniform_buffer_reflection <T, L>> : std::true_type {};

template <typename T, template <typename> typename L>
struct is_global_resource_reflection <storage_buffer_reflection <T, L>> : std::true_type {};

template <typename T, size_t D>
struct is_global_resource_reflection <sampler_reflection <T, D>> : std::true_type {};

template <typename T>
constexpr bool is_global_resource_reflection_v = is_global_resource_reflection <T> ::value;

template <typename T>
constexpr bool is_global_resource_v = is_global_resource_reflection <typename T::reflection> ::value;

// Attribute stream detection
template <typename T>
struct is_attribute_stream_reflection : std::false_type {};

template <typename T, vk::VertexInputRate R>
struct is_attribute_stream_reflection <attribute_stream_reflection <T, R>> : std::true_type {};

template <typename T>
constexpr bool is_attribute_stream_reflection_v = is_attribute_stream_reflection <T> ::value;

template <typename T>
constexpr bool is_attribute_stream_v =
	has_reflection <T> () && is_attribute_stream_reflection_v <typename T::reflection>;
