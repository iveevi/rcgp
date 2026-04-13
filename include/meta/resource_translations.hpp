#pragma once

#include "resources.hpp"

namespace vk {

struct AccelerationStructureHKR;

} // namespace vk

namespace rcgp {

// Forward declarations
template <typename T, template <typename> typename L, vk::BufferUsageFlagBits F>
struct MirrorBuffer;

struct Image;
struct MirrorSampler;
struct RaytracingAccelerationStructure;

template <auto &target_ref>
requires is_render_target_v <reference_base_of <target_ref>>
struct TargetMirrorSampler;

// Resource translation mappings
template <typename T>
struct resource_translation {};

template <typename T>
using resource_translation_handle_t = resource_translation <T> ::handle_type;

template <typename T, template <typename> typename L, vk::VertexInputRate R>
struct resource_translation <AttributeStream <T, L, R>> {
	using handle_type = MirrorBuffer <array <T>, L, vk::BufferUsageFlagBits::eVertexBuffer>;
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
	using handle_type = MirrorBuffer <T, L, vk::BufferUsageFlagBits::eUniformBuffer>;
	using host_type = handle_type::value_type;
};

template <typename T, template <typename> typename L, GlobalResourceAccess A>
struct resource_translation <StorageBuffer <T, L, A>> {
	using handle_type = MirrorBuffer <T, L, vk::BufferUsageFlagBits::eStorageBuffer>;
	using host_type = handle_type::value_type;
};

template <typename T, size_t D>
struct resource_translation <Sampler <T, D>> {
	using handle_type = MirrorSampler;
	using host_type = std::nullptr_t;
};

template <auto &target_ref>
struct resource_translation <sampler <target_ref>> {
	using handle_type = TargetMirrorSampler <target_ref>;
	using host_type = std::nullptr_t;
};

template <typename T, size_t D, GlobalResourceAccess A>
struct resource_translation <StorageImage <T, D, A>> {
	// TODO: need dimension
	using handle_type = Image;
	using host_type = std::nullptr_t;
};

template <>
struct resource_translation <RaytracingAccelerationStructure> {
	using handle_type = vk::AccelerationStructureKHR;
	using host_type = std::nullptr_t;
};

template <typename R, int64_t D>
requires is_global_resource_v <R>
struct resource_translation <array <R, D>> {
	using base_handle_type = resource_translation <R> ::handle_type;
	using handle_type = std::array <base_handle_type, D>;
	using host_type = std::nullptr_t;
};

template <typename R>
requires is_global_resource_v <R>
struct resource_translation <array <R, -1>> {
	using base_handle_type = resource_translation <R> ::handle_type;
	using handle_type = std::vector <base_handle_type>;
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
