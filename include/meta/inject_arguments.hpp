#pragma once

#include "../dsl/instructions.hpp"
#include "../dsl/jems.hpp"
#include "implicit_context.hpp"
#include "inject_reference.hpp"
#include "reconstruct_type.hpp"
#include "reference.hpp"
#include "reflection.hpp"
#include "shader_stage.hpp"
#include "static_string.hpp"

struct InjectionCounters {
	size_t argidx;
	size_t threadidx;
};

// Fallback case with error reported
template <ShaderStage S, typename T>
void inject_one_argument(T &value, InjectionCounters &counters)
{
	// TODO: also ss for ShaderStage
	static_assert(false, ($ss("argument injector not implemented for ") + $ss_type(T)).view());
}

// Nothing to do for the implicit context
template <ShaderStage S, auto &... refs>
void inject_one_argument(implicit_context <refs...> &value, InjectionCounters &counters) {}

// TODO: just take void * input...
template <aggregate T, size_t I>
void inject_resource_group_element(void *addr, T &value)
{
	auto &field = value.template _ugp_field_reference <I> ();
	using R = std::decay_t <decltype(field)> ::reflection;

	auto grsrc = R::intrinsic(I);
	$tsb.context.add_global_resource(addr, grsrc);
	inject_reference(field, grsrc);
}

template <aggregate T, size_t ... Is>
void inject_resource_group_carrier(void *addr, T &value, std::index_sequence <Is...>)
{
	(inject_resource_group_element <T, Is> (addr, value), ...);
}

template <typename T, T &ref>
void inject_resource_reference(reference <ref> &value)
{
	if constexpr (is_resource_group_v <T>) {
		// Resource groups
		using U = T::reflection::value_type;
		using R = expand_reflection_t <U>;

		// TODO: defer to overloads to handle tuple vs aggregates
		static_assert(is_aggregate_reflection_v <R>);
		using V = R::original_type;
		
		static constexpr auto N = R::field_count;
		inject_resource_group_carrier(
			(void *) &ref,
			static_cast <V &> (value),
			std::make_index_sequence <N> ()
		);
	} else if constexpr (is_global_resource_v <T>) {
		// Global resources
		using R = typename T::reflection;
		auto grsrc = R::intrinsic(0);
		$tsb.context.add_global_resource((void *) &ref, grsrc);
		inject_reference(static_cast <T &> (value), grsrc);
	} else {
		// Unknown cases
		static_assert(false, ($ss("resource reference injector not implemented for ") + $ss_type(T)).view());
	}
}

// Broader case with stage information
template <ShaderStage S, auto &ref>
void inject_one_argument(reference <ref> &value, InjectionCounters &counters)
{
	using R = reference_base_t <ref>;

	if constexpr (S == ShaderStage::eVertex && is_attribute_stream_v <R>) {
		// Attribute streams for vertex shaders
		using reflection = R::reflection;
		using T = reflection::value_type;

		auto type = reconstruct_type <T> ();

		// TODO: this can be a method; similar for argument, if its used many times
		auto tin = ThreadInput(type, counters.threadidx++);
		$tsb.context.add_thread_input(tin);
		inject_reference(static_cast <T &> (value), jems::thread_input(tin));
	} else {
		// Regular case
		inject_resource_reference(value);
	}
}

template <ShaderStage S, typename T>
requires (primitive <T> or aggregate <T>)
void inject_one_argument(T &value, InjectionCounters &counters)
{
	auto type = reconstruct_type <T> ();
	if constexpr (S == ShaderStage::eFragment) {
		// Varying attribute
		auto tin = ThreadInput(type, counters.threadidx++);
		$tsb.context.add_thread_input(tin);
		inject_reference(value, jems::thread_input(tin));
	} else if constexpr (S == ShaderStage::eSubroutine) {
		// Function argument
		auto arg = Argument(type, counters.argidx++);
		$tsb.context.add_argument(arg);
		inject_reference(value, jems::argument(arg));
	} else {
		// Not supported
		static_assert(false, (
			$ss("argument type ")
			+ $ss_type(T)
			+ $ss(" is not supported in Stage X")
		).view());
	}
}

// Top-level processors
template <ShaderStage S, typename ... Args, size_t ... Is>
void inject_all_arguments(std::tuple <Args...> &args, std::index_sequence <Is...>)
{
	auto counters = InjectionCounters(0, 0);
	(inject_one_argument <S> (std::get <Is> (args), counters), ...);
}

template <ShaderStage S, typename ... Args>
void inject_arguments(std::tuple <Args...> &args)
{
	static constexpr size_t N = sizeof...(Args);
	return inject_all_arguments <S> (args, std::make_index_sequence <N> ());
}
