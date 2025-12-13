#pragma once

#include "../dsl/instructions.hpp"
#include "../dsl/jems.hpp"
#include "implicit_context.hpp"
#include "injection_state.hpp"
#include "injector.hpp"
#include "reference.hpp"
#include "reflection.hpp"
#include "reconstruct_type.hpp"
#include "resource_injector.hpp"
#include "shader_stage.hpp"
#include "stage_intrinsics.hpp"
#include "static_string.hpp"

template <ShaderStage S>
void inject_execution_model()
{
	if constexpr (S == ShaderStage::Vertex)
		$tsb.context.model = ExecutionModel::eVulkanVertex;
	else if constexpr (S == ShaderStage::Fragment)
		$tsb.context.model = ExecutionModel::eVulkanFragment;
	else
		static_assert(false, "no execution model for stage");
}

template <ShaderStage S, typename T>
struct stage_argument_injector {
	static auto main(T &, const InjectionState &state) {
		static_assert(false, ($ss("stage argument injector not implemented for ") + $ss_type(T)).view());
		return state;
	}
};

// Intrinsics never need injection
template <ShaderStage S, GlobalIntrinsic G, typename T>
struct stage_argument_injector <S, read_only_intrinsic <G, T>> {
	static auto main(read_only_intrinsic <G, T> &, const InjectionState &state) {
		return state;
	}
};

// For subroutines, normal arguments are treated like regular arguments
template <typename T>
struct stage_argument_injector <ShaderStage::Undefined, T> {
	static auto main(T &value, const InjectionState &state) {
		auto type = reconstruct_type <T> ();
		auto arg = Argument(type, state.argidx);
		$tsb.context.add_argument(arg);
		injector <T> ::main(value, jems::argument(arg));
		return state.next(true, false);
	}
};

// For all stages, handling resource references is basically the same
template <ShaderStage S, auto &rsrc>
struct stage_argument_injector <S, reference <rsrc>> {
	using T = std::decay_t <decltype(rsrc)>;

	static auto main(reference <rsrc> &value, const InjectionState &state) {
		return resource_injector <T, rsrc> ::main(value, state);
	}
};

// For fragment shaders primitive arguments are thread inputs;
// 	vertex stage is included for simplicity of implementation,
// 	but users will not be allowed to write vertex shaders without
// 	explicit references to the streams
template <ShaderStage S, primitive T>
requires (S == ShaderStage::Vertex || S == ShaderStage::Fragment)
struct stage_argument_injector <S, T> {
	static auto main(T &value, const InjectionState &state) {
		auto type = reconstruct_type <T> ();
		auto tin = ThreadInput(type, state.threadidx);
		$tsb.context.add_thread_input(tin);
		injector <T> ::main(value, jems::thread_input(tin));
		return state.next(false, true);
	}
};

// For vertex shaders streams are thread inputs
template <reflected T, template <typename> typename L, vk::VertexInputRate R, AttributeStream <T, L, R> &rsrc>
struct stage_argument_injector <ShaderStage::Vertex, reference <rsrc>> {
	static auto main(reference <rsrc> &value, const InjectionState &state) {
		return stage_argument_injector <ShaderStage::Vertex, T> ::main(value, state);
	}
};

template <ShaderStage S, aggregate T>
requires (S == ShaderStage::Vertex || S == ShaderStage::Fragment)
struct stage_argument_injector <S, T> {
	static auto main(T &value, const InjectionState &state) {
		constexpr auto field_count = T::reflection::field_count;

		auto next = state;
		auto proc = [&](auto el) {
			constexpr auto idx = decltype(el)::value;
			auto &fvalue = value.template _ugp_field_reference <idx> ();
			using F = std::decay_t <decltype(fvalue)>;
			next = stage_argument_injector <S, F> ::main(fvalue, next);
			return true;
		};

		auto els = el_series(std::make_index_sequence <field_count> ());
		std::apply([&](auto ... xs) {
			std::make_tuple(proc(xs)...);
		}, els);

		return next;
	}
};

// Always ignore the implicit context
template <ShaderStage S, auto & ... refs>
struct stage_argument_injector <S, implicit_context <refs...>> {
	static auto main(auto &, const InjectionState &state) {
		return state.next(false, false);
	}
};

template <ShaderStage S, size_t Index, typename ... Args>
void inject_arguments(std::tuple <Args...> &args, const InjectionState &state)
{
	auto &value = std::get <Index> (args);
	using T = std::decay_t <decltype(value)>;

	auto next = stage_argument_injector <S, T> ::main(value, state);
	if constexpr (Index + 1 < sizeof...(Args))
		inject_arguments <S, Index + 1> (args, next);
}
