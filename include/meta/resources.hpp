#pragma once

#include <vulkan/vulkan.hpp>

#include "../dsl/jems.hpp"
#include "../util/error.hpp"
#include "../util/cti.hpp"
#include "layouts.hpp"

namespace rcgp {

struct resource_handle {
	void override_reference(const Reference &ref) {
		fatal("attempting to override raw resource handle");
	}
};

template <
	typename T,
	template <typename> typename L = layouts::scalar,
	vk::VertexInputRate R = vk::VertexInputRate::eVertex
> struct AttributeStream {
	using handle_type = T;
};

template <
	typename T,
	template <typename> typename L = layouts::std430
> struct PushConstant : resource_handle {
	using handle_type = T;
};

template <
	typename T,
	template <typename> typename L = layouts::std430
> struct UniformBuffer : resource_handle {
	using handle_type = T;
};

template <
	typename T,
	template <typename> typename L = layouts::std430,
	GlobalResourceAccess A = GlobalResourceAccess::eReadWrite
> struct StorageBuffer : resource_handle {
	using handle_type = T;
};

template <native_scalar T, size_t D>
struct Sampler : resource_handle {
	struct handle_type : jems::handle {
		auto sample(const vector <T, D> &x, $location) const {
			auto result = jems::builtin_intrinsic(
				BuiltinIntrinsicCode::eSample,
				std::vector <Reference> { *this, x },
				loc
			);

			return vector <T, 4> ::reinterpret(result);
		}
	};
};

// TODO: something similar for descriptor heaps with strided access

TYPE_TRAIT(is_global_resource);

template <typename T, template <typename> typename L>
TYPE_TRAIT_INCLUDES(is_global_resource, PushConstant <T, L>);

template <typename T, template <typename> typename L>
TYPE_TRAIT_INCLUDES(is_global_resource, UniformBuffer  <T, L>);

template <typename T, template <typename> typename L, GlobalResourceAccess A>
TYPE_TRAIT_INCLUDES(is_global_resource, StorageBuffer  <T, L, A>);

template <typename T, size_t D>
TYPE_TRAIT_INCLUDES(is_global_resource, Sampler <T, D>);

namespace detail {

template <typename T>
struct resource_group_builder {};

template <typename T>
using resource_group_t = resource_group_builder <T> ::type;

// TODO: global_resource concept?
template <typename T>
requires is_global_resource_v <T>
struct resource_group_builder <T> {
	using type = T::handle_type;
};

template <user_defined T>
struct resource_group_builder <T> {
	using translated = T::fields::template map <resource_group_t>;
	using hints = translated::template map <scaffold_natural>;
	using type = scaffold_lookup <scaffold_natural <hints>, T> ::type;
};

} // namespace detail

template <user_defined T>
struct ResourceGroup : T {
	using struct_type = T;
	using handle_type = detail::resource_group_t <T>;
};

template <typename T>
TYPE_TRAIT_INCLUDES(is_global_resource, ResourceGroup <T>);

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

TYPE_TRAIT(is_uniform_buffer);
	template <typename T, template <typename> typename L>
	TYPE_TRAIT_INCLUDES(is_uniform_buffer, UniformBuffer <T, L>);

TYPE_TRAIT(is_sampler);
	template <typename T, size_t D>
	TYPE_TRAIT_INCLUDES(is_sampler, Sampler <T, D>);

} // namespace rcgp
