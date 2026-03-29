#pragma once

#include <vulkan/vulkan.hpp>

#include "../dsl/jems.hpp"
#include "../dsl/array.hpp"
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

template <native_scalar T, size_t D, GlobalResourceAccess A = GlobalResourceAccess::eRead>
struct StorageImage : resource_handle {
	struct handle_type : jems::handle {
		void write(vector <int32_t, D> idx, vector <T, 4> value, $location) {
			jems::builtin_intrinsic(
				BuiltinIntrinsicCode::eImageStore,
				{ *this, idx, value }, loc
			);
		}
	};
};

template <native_scalar T, size_t D>
struct Sampler : resource_handle {
	struct handle_type : jems::handle {
		auto sample(const vector <T, D> &x, $location) const {
			auto result = jems::builtin_intrinsic(
				BuiltinIntrinsicCode::eSample,
				{ *this, x }, loc
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
	
	template <typename T, size_t D, GlobalResourceAccess A>
	TYPE_TRAIT_INCLUDES(is_global_resource, StorageImage <T, D, A>);

	template <typename T, size_t D>
	TYPE_TRAIT_INCLUDES(is_global_resource, Sampler <T, D>);

	template <typename R, int64_t D>
	requires is_global_resource_v <R>
	TYPE_TRAIT_INCLUDES(is_global_resource, array <R, D>);

// TODO: this should not include resource groups...
template <typename R, int64_t D>
requires is_global_resource_v <R>
struct array <R, D> : resource_handle {
	using base = R;
	static constexpr int64_t elements = D;

	struct handle_type : jems::handle {
		auto operator[](const uint32_t &index, $location) const {
			auto result = typename R::handle_type();
			auto access = jems::array_access(*this, scalar <int32_t> (index), loc);
			result.override_reference(access);
			return result;
		}

		// TODO: automatic non-uniform conversion
		auto operator[](const scalar <uint32_t> &index, $location) const {
			auto result = typename R::handle_type();
			auto access = jems::array_access(*this, index, loc);
			result.override_reference(access);
			return result;
		}
	};

	static const inline R element;
};

namespace detail {

template <typename T>
struct resource_group_builder {};

template <typename T>
using resource_group_t = resource_group_builder <T> ::type;

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

// Render targets
struct ColorTarget : resource_handle { struct handle_type {}; };
struct DepthTarget : resource_handle { struct handle_type {}; };

TYPE_TRAIT(is_color_target);
	template <>
	TYPE_TRAIT_INCLUDES(is_color_target, ColorTarget);

TYPE_TRAIT(is_depth_target);
	template <>
	TYPE_TRAIT_INCLUDES(is_depth_target, DepthTarget);

TYPE_TRAIT(is_render_target);
	template <>
	TYPE_TRAIT_INCLUDES(is_render_target, ColorTarget);
	template <>
	TYPE_TRAIT_INCLUDES(is_render_target, DepthTarget);

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

TYPE_TRAIT(is_storage_image);
	template <typename T, size_t D, GlobalResourceAccess A>
	TYPE_TRAIT_INCLUDES(is_storage_image, StorageImage <T, D, A>);

TYPE_TRAIT(is_resource_array);
	template <typename R, int64_t D>
	requires is_global_resource_v <R>
	TYPE_TRAIT_INCLUDES(is_resource_array, array <R, D>);

} // namespace rcgp
