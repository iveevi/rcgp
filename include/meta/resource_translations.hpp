#pragma once

#include "resources.hpp"

namespace rcgp {

// Forward declarations
template <typename T, template <typename> typename L, VkBufferUsageFlagBits F>
struct MirrorBuffer;

struct MirrorSampler;

// Resource translation mappings
template <typename T>
struct resource_translation {};

template <typename T>
using resource_translation_handle_t = resource_translation <T> ::handle_type;

template <typename T, template <typename> typename L, VkVertexInputRate R>
struct resource_translation <AttributeStream <T, L, R>> {
	using handle_type = MirrorBuffer <array <T>, L, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT>;
	using host_type = handle_type::value_type;
};

template <typename T, template <typename> typename L>
struct resource_translation <PushConstant <T, L>> {
	using hint = L <T> ::hint;
	using handle_type = scaffold_lookup <hint, T, true> ::type;
	using host_type = handle_type;
};

template <typename T, template <typename> typename L>
struct resource_translation <UniformBuffer <T, L>> {
	using handle_type = MirrorBuffer <T, L, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT>;
	using host_type = handle_type::value_type;
};

template <typename T, template <typename> typename L, GlobalResourceAccess A>
struct resource_translation <StorageBuffer <T, L, A>> {
	using handle_type = MirrorBuffer <T, L, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT>;
	using host_type = handle_type::value_type;
};

template <typename T, size_t D>
struct resource_translation <Sampler <T, D>> {
	using handle_type = MirrorSampler;
	using host_type = std::nullptr_t;
};

template <user_defined T>
struct resource_translation <ResourceGroup <T>> {
	using translated = T::fields::template map <resource_translation_handle_t>;
	using hints = translated::template map <scaffold_natural>;
	using handle_type = scaffold_lookup <scaffold_natural <hints>, T> ::type;
	using host_type = std::nullptr_t;
};

} // namespace rcgp
