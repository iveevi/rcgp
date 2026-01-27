#pragma once

#include "../dsl/jems.hpp"
#include "../util/cti.hpp"
#include "layouts.hpp"

namespace rcgp {

// Corresponds to vertex buffes
template <typename T, template <typename> typename L = layouts::scalar, vk::VertexInputRate R = vk::VertexInputRate::eVertex>
struct AttributeStream : T {
	using value_type = T;
};

// Resource groups
// TODO: should not allow index/vertex buffers here
template <typename T>
struct ResourceGroup : T {
	using value_type = T;
};

// Push constants
template <typename T, template <typename> typename L = layouts::std430>
struct PushConstant : T {};

// Uniform buffers
template <typename T, template <typename> typename L = layouts::std430>
struct UniformBuffer : T {};

// TODO: Read/write flags and aliases...
template <typename T, template <typename> typename L = layouts::std430, GlobalResourceAccess A = GlobalResourceAccess::eReadWrite>
struct StorageBuffer : T {};

// TODO: aliases in a different header
template <typename T, template <typename> typename L = layouts::std430, GlobalResourceAccess A = GlobalResourceAccess::eReadWrite>
using ArrayBuffer = StorageBuffer <array <T>, L, A>;

template <typename T, template <typename> typename L = layouts::std430>
using RStorageBuffer = StorageBuffer <T, L, GlobalResourceAccess::eRead>;

template <typename T, template <typename> typename L = layouts::std430>
using WStorageBuffer = StorageBuffer <T, L, GlobalResourceAccess::eWrite>;

template <typename T, template <typename> typename L = layouts::std430>
using RWStorageBuffer = StorageBuffer <T, L, GlobalResourceAccess::eReadWrite>;

template <native_scalar T, size_t D>
struct Sampler : jems::handle {
	auto sample(vector <T, D> x, $location) const
	requires native_float_scalar <T> {
		auto result = jems::builtin_intrinsic_loc(
			loc, BuiltinIntrinsicCode::eSample,
			*this, x
		);

		return vector <T, 4> ::reinterpret(result);
	}
};

// Aliases for the common case
using Sampler1D = Sampler <float, 1>;
using Sampler2D = Sampler <float, 2>;
using Sampler3D = Sampler <float, 3>;

// Type traits for these resources
TYPE_TRAIT(is_attribute_stream);
	template <typename T, template <typename> typename L, vk::VertexInputRate R>
	TYPE_TRAIT_INCLUDES(is_attribute_stream, AttributeStream <T, L, R>);

TYPE_TRAIT(is_resource_group);
	template <typename T>
	TYPE_TRAIT_INCLUDES(is_resource_group, ResourceGroup <T>);

TYPE_TRAIT(is_push_constant);
	template <typename T, template <typename> typename L>
	TYPE_TRAIT_INCLUDES(is_push_constant, PushConstant <T, L>);

TYPE_TRAIT(is_storage_buffer);
	template <typename T, template <typename> typename L, GlobalResourceAccess A>
	TYPE_TRAIT_INCLUDES(is_storage_buffer, StorageBuffer <T, L, A>);

TYPE_TRAIT(is_sampler);
	template <typename T, size_t D>
	TYPE_TRAIT_INCLUDES(is_sampler, Sampler <T, D>);

TYPE_TRAIT(is_global_resource);
	template <typename T>
	TYPE_TRAIT_INCLUDES(is_global_resource, ResourceGroup <T>);

	template <typename T, template <typename> typename L>
	TYPE_TRAIT_INCLUDES(is_global_resource, PushConstant <T, L>);
	
	template <typename T, template <typename> typename L>
	TYPE_TRAIT_INCLUDES(is_global_resource, UniformBuffer  <T, L>);
	
	template <typename T, template <typename> typename L, GlobalResourceAccess A>
	TYPE_TRAIT_INCLUDES(is_global_resource, StorageBuffer  <T, L, A>);
	
	template <typename T, size_t D>
	TYPE_TRAIT_INCLUDES(is_global_resource, Sampler <T, D>);

} // namespace rcgp
