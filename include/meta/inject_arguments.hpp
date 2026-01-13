#pragma once

#include "../dsl/instructions.hpp"
#include "../dsl/jems.hpp"
#include "implicit_context.hpp"
#include "inject_reference.hpp"
#include "reconstruct_type.hpp"
#include "reference.hpp"
#include "reflection.hpp"
#include "shader_stage.hpp"
#include "stage_intrinsics.hpp"
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

template <ShaderStage S, uint32_t X, uint32_t Y, uint32_t Z>
void inject_one_argument(WorkGroup <X, Y, Z> &, InjectionCounters &)
{
	static_assert(
		S == ShaderStage::eCompute
		|| S == ShaderStage::eMesh,
		"WorkGroup is only valid for compute/mesh shaders"
	);
	$tsb.context.set_workgroup_size(X, Y, Z);
}

template <ShaderStage S, uint32_t X, uint32_t Y, uint32_t Z>
void inject_one_argument(TaskGroup <X, Y, Z> &, InjectionCounters &)
{
	static_assert(
		S == ShaderStage::eTask,
		"TaskGroup is only valid for task shaders"
	);
	$tsb.context.set_workgroup_size(X, Y, Z);
}

// Nothing to do for the implicit context
template <ShaderStage S, auto &... refs>
void inject_one_argument(implicit_context <refs...> &value, InjectionCounters &counters) {}

template <ShaderStage S, reflected T>
void inject_one_argument(TaskPayload <T> &value, InjectionCounters &)
{
	static_assert(S == ShaderStage::eMesh, "TaskPayload is only valid for mesh shaders");
	$tsb.context.task_payload_type = reconstruct_type <T> ();
	inject_reference(Tas <T &> (value), jems::global_intrinsic(GlobalIntrinsic::eTaskPayload));
}

template <aggregate T, size_t I>
void inject_resource_group_element(void *addr, T &value)
{
	auto &field = value.template _rcgp_get <I> ();
	using R = std::decay_t <decltype(field)>;

	auto grsrc = resource_intrinsic(R(), I);
	$tsb.context.add_global_resource(addr, grsrc);
	inject_reference(field, grsrc);
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

		constexpr_for(Is, R::field_count,
			(inject_resource_group_element <V, Is> (&ref, value), ...)
		);
	} else if constexpr (is_global_resource_v <T>) {
		// Global resources
		auto grsrc = resource_intrinsic(T(), 0);
		$tsb.context.add_global_resource(&ref, grsrc);
		inject_reference(Tas <T &> (value), grsrc);
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
		inject_reference(Tas <T &> (value), jems::thread_input(tin));
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

// Top-level processor
template <ShaderStage S, typename ... Args>
void inject_arguments(std::tuple <Args...> &args)
{
	auto counters = InjectionCounters(0, 0);
	constexpr_for(Is, sizeof...(Args),
		(inject_one_argument <S> (std::get <Is> (args), counters), ...)
	);
}
