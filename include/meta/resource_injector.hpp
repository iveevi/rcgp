#pragma once

#include <cstdint>
#include <tuple>
#include <type_traits>

#include "expand_reflection.hpp"
#include "injection_state.hpp"
#include "injector.hpp"
#include "resource_intrinsic.hpp"
#include "resources.hpp"
#include "reference.hpp"
#include "static_string.hpp"

// Pre-pass for resource group injectors
template <typename Resources, typename ... Ts>
struct resource_group_collect {
	using result = Resources;
};

// Injectors for various resources
template <typename T, T &rsrc>
struct resource_injector {
	static auto main(reference <rsrc> &, const InjectionState &state) {
		static_assert(false, ($ss("reference injector not implemented for type ") + $ss_type(T)).view());
		return state.next(false, false);
	}
};

template <reflected T, template <typename> typename L, UniformBuffer <T, L> &rsrc>
struct resource_injector <UniformBuffer <T, L>, rsrc> {
	static auto main(reference <rsrc> &value, const InjectionState &state) {
		auto grsrc = resource_intrinsic <uniform_buffer_reflection <T, L>> ::intrinsic(0);
		$tsb.context.add_global_resource <rsrc> (grsrc);
		injector <T> ::main(value, grsrc);
		return state.next(false, false);
	}
};

template <reflected T, template <typename> typename L, StorageBuffer <T, L> &rsrc>
struct resource_injector <StorageBuffer <T, L>, rsrc> {
	static auto main(reference <rsrc> &value, const InjectionState &state) {
		auto grsrc = resource_intrinsic <storage_buffer_reflection <T, L>> ::intrinsic(0);
		$tsb.context.add_global_resource <rsrc> (grsrc);
		injector <T> ::main(value, grsrc);
		return state.next(false, false);
	}
};

template <typename T, size_t D, Sampler <T, D> &rsrc>
struct resource_injector <Sampler <T, D>, rsrc> {
	static auto main(reference <rsrc> &value, const InjectionState &state) {
		auto grsrc =  resource_intrinsic <sampler_reflection <T, D>> ::intrinsic(0);
		$tsb.context.add_global_resource <rsrc> (grsrc);
		value.ref = grsrc;
		return state.next(false, false);
	}
};

template <typename T, ResourceGroup <T> &rsrc>
struct resource_injector <ResourceGroup <T>, rsrc> {
	static auto main(reference <rsrc> &value, const InjectionState &state) {
		using R = expand_reflection_t <T>;
		using Resources = typename resource_group_collect <
			sequence <>,
			field_trace <R>
		> ::result;

		uint32_t counter = 0;

		// Process any resources
		auto proc = [&](auto x) {
			using trace = std::decay_t <decltype(x)>;
			using resource = typename trace::value_type;
			auto grsrc = resource_intrinsic <resource> ::intrinsic(counter++);
			$tsb.context.add_global_resource <rsrc> (grsrc);
			injector <trace> ::main(value, grsrc);
			return true;
		};

		std::apply([&](auto ... xs) { (proc(xs), ...); }, typename Resources::tuple());

		return state.next(false, false);
	}
};
