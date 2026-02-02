#pragma once

#include "../dsl/jems.hpp"
#include "implicit_context.hpp"
#include "inject_reference.hpp"
#include "reconstruct_type.hpp"
#include "reference.hpp"
#include "stage_intrinsics.hpp"
#include "static_string.hpp"

namespace rcgp {

struct InjectionCounters {
	size_t argidx;
	size_t threadidx;
};

// nullptr type is a freebie
template <ShaderStage S>
void inject_one_argument(std::nullptr_t &value, InjectionCounters &counters) {}

// Fallback case with error reported
template <ShaderStage S, typename T>
void inject_one_argument(T &value, InjectionCounters &counters)
{
	// TODO: static strings for ShaderStage
	static_error("argument injector not implemented for "_ss + $ss_type(T));
}

template <ShaderStage S, uint32_t X, uint32_t Y, uint32_t Z>
void inject_one_argument(WorkGroup <X, Y, Z> &, InjectionCounters &)
{
	// TODO: use if not constexpr...
	static_assert(
		S == ShaderStage::eCompute
		|| S == ShaderStage::eMesh,
		"WorkGroup is only valid for compute/mesh shaders"
	);
	$tsb.set_workgroup_size(X, Y, Z);
}

template <ShaderStage S, uint32_t X, uint32_t Y, uint32_t Z>
void inject_one_argument(TaskGroup <X, Y, Z> &, InjectionCounters &)
{
	static_assert(
		S == ShaderStage::eTask,
		"TaskGroup is only valid for task shaders"
	);
	$tsb.set_workgroup_size(X, Y, Z);
}

// TODO: check stage compatibility
template <ShaderStage S1, GlobalIntrinsic G, ShaderStage S2, typename T>
void inject_one_argument(read_only_intrinsic <G, S2, T> &value, InjectionCounters &counters)
{
	if constexpr (S1 != S2) {
		// TODO: static string for global intrinsics...
		static_error("unsupported intrinsic"_ss);
	}
}

template <ShaderStage S1, GlobalIntrinsic G, ShaderStage S2, typename T>
void inject_one_argument(write_only_intrinsic <G, S2, T> &value, InjectionCounters &counters)
{
	if constexpr (S1 != S2)
		static_error("unsupported intrinsic"_ss);
}

// Nothing to do for the implicit context
template <ShaderStage S, auto &... refs>
void inject_one_argument(implicit_context <refs...> &value, InjectionCounters &counters) {}

template <ShaderStage S, typename T>
void inject_one_argument(TaskPayload <T> &value, InjectionCounters &)
{
	static_assert(S == ShaderStage::eMesh, "TaskPayload is only valid for mesh shaders");
	$tsb.task_payload_type = reconstruct_type <T> ();
	inject_reference(Tas <T &> (value), jems::global_intrinsic(GlobalIntrinsic::eTaskPayload));
}

template <aggregate T, size_t I>
void inject_resource_group_element(void *addr, T &value)
{
	auto &field = value.template _rcgp_get <I> ();
	using R = std::decay_t <decltype(field)>;

	auto grsrc = resource_intrinsic(R(), I);
	$tsb.add_global_resource(addr, grsrc);
	inject_reference(field, grsrc);
}

template <typename T, T &ref>
void inject_resource_reference(reference <ref> &value)
{
	if constexpr (is_resource_group_v <T>) {
		// TODO: assert that its an aggregate
		using U = T::value_type;
		constexpr_for(Is, U::field_count,
			(inject_resource_group_element <U, Is> (&ref, value), ...)
		);
	} else if constexpr (is_global_resource_v <T>) {
		// Global resources
		auto grsrc = resource_intrinsic(T(), 0);
		$tsb.add_global_resource(&ref, grsrc);
		inject_reference(Tas <T &> (value), grsrc);
	} else {
		// Unknown cases
		static_error("resource reference injector not implemented for "_ss + $ss_type(T));
	}
}

// Broader case with stage information
template <ShaderStage S, auto &ref>
void inject_one_argument(reference <ref> &value, InjectionCounters &counters)
{
	using R = reference_base_t <ref>;

	if constexpr (is_attribute_stream_v <R>) {
		// TODO: move to the else branch with a false static assert
		static_assert(
			S == ShaderStage::eVertex || S == ShaderStage::eSubroutine,
			"AttributeStream is only valid for vertex or subroutine shaders"
		);

		// Attribute streams for vertex shaders or subroutines
		using T = R::value_type;

		auto type = reconstruct_type <T> ();

		// TODO: this can be a method; similar for argument, if its used many times
		if constexpr (S == ShaderStage::eSubroutine) {
			auto arg = Argument(type, counters.argidx++);
			$tsb.add_argument(arg);
			inject_reference(Tas <T &> (value), jems::argument(arg));
		} else {
			auto tin = StageInput(type, counters.threadidx++);
			$tsb.add_stage_input(tin);
			inject_reference(Tas <T &> (value), jems::stage_input(tin));
		}
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
		// TODO: aggregate case is a bit different
		// Varying attribute
		auto tin = StageInput(type, counters.threadidx++);
		$tsb.add_stage_input(tin);
		inject_reference(value, jems::stage_input(tin));
	} else if constexpr (S == ShaderStage::eSubroutine) {
		// Function argument
		auto arg = Argument(type, counters.argidx++);
		$tsb.add_argument(arg);
		inject_reference(value, jems::argument(arg));
	} else {
		// Not supported
		static_error("argument type "_ss + $ss_type(T)
			+ " is not supported in Stage X"_ss);
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

} // namespace rcgp
