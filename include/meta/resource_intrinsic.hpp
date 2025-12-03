#pragma once

#include <cstdint>

#include "../dsl/jems.hpp"
#include "reflection.hpp"
#include "reconstruct_type.hpp"
#include "static_string.hpp"

template <typename T>
struct resource_intrinsic {
	static auto intrinsic(uint32_t) {
		static_assert(false, ($ss("resource intrinsic generator not implemented for ") + $ss_type(T)).view());
	}
};

template <typename T>
struct resource_intrinsic <structured_buffer_reflection <T>> {
	static auto intrinsic(uint32_t binding) {
		auto type = reconstruct_type <T> ();
		auto grsrc = jems::global_resource(type, GlobalResource::eStorageBuffer, std::nullopt, binding);
		return grsrc;
	}
};

template <typename T, size_t D>
struct resource_intrinsic <sampler_reflection <T, D>> {
	static auto intrinsic(uint32_t binding) {
		auto type = jems::type(VectorType <T, D> ());
		auto grsrc = jems::global_resource(type, GlobalResource::eSampler, std::nullopt, binding);
		return grsrc;
	}
};
