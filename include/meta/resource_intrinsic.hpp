#pragma once

#include <cstdint>
#include <type_traits>

#include "../dsl/jems.hpp"
#include "../dsl/primitives.hpp"
#include "layout/all.hpp"
#include "reflection.hpp"
#include "reconstruct_type.hpp"
#include "static_string.hpp"

template <typename T>
struct resource_intrinsic {
	static auto intrinsic(uint32_t) {
		static_assert(false, ($ss("resource intrinsic generator not implemented for ") + $ss_type(T)).view());
	}
};

template <template <typename> typename L>
consteval GlobalResource::Layout layout_of()
{
	using sample = primitive_reflection <scalar <int>>;
	if constexpr (std::is_same_v <L <sample>, layouts::std430 <sample>>)
		return GlobalResource::Layout::eStd430;
	else if constexpr (std::is_same_v <L <sample>, layouts::scalar <sample>>)
		return GlobalResource::Layout::eScalar;
	else
		static_assert(false, "unsupported layout for global resource");
}

template <typename T, template <typename> typename L>
struct resource_intrinsic <uniform_buffer_reflection <T, L>> {
	static auto intrinsic(uint32_t binding) {
		auto type = reconstruct_type <T> ();
		auto grsrc = jems::global_resource(type, GlobalResource::eUniformBuffer, layout_of <L> (), std::nullopt, binding);
		return grsrc;
	}
};

template <typename T, template <typename> typename L>
struct resource_intrinsic <storage_buffer_reflection <T, L>> {
	static auto intrinsic(uint32_t binding) {
		auto type = reconstruct_type <T> ();
		auto grsrc = jems::global_resource(type, GlobalResource::eStorageBuffer, layout_of <L> (), std::nullopt, binding);
		return grsrc;
	}
};

template <typename T, size_t D>
struct resource_intrinsic <sampler_reflection <T, D>> {
	static auto intrinsic(uint32_t binding) {
		auto type = jems::type(VectorType <T, D> ());
		auto grsrc = jems::global_resource(type, GlobalResource::eSampler, GlobalResource::Layout::eUnknown, std::nullopt, binding);
		return grsrc;
	}
};
