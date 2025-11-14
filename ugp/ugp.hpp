#pragma once

#include <functional>

#include "msv/context.hpp"
#include "msv/function_return_injection.hpp"
#include "msv/reference.hpp"
#include "msv/reflection.hpp"
#include "msv/resources.hpp"
#include "msv/stage.hpp"
#include "msv/stage_intrinsics.hpp"
#include "msv/static_string.hpp"
#include "msv/this_injection.hpp"
#include "msv/type_hash.hpp"
#include "msv/type_string_extensions.hpp"
#include "msv/reconstruct_type.hpp"

#include "rhi/session.hpp"
#include "rhi/device.hpp"

#include "dsl/tracer.hpp"
#include "dsl/jems.hpp"
#include "dsl/generators/assembly.hpp"
#include "dsl/generators/glsl.hpp"

template <typename R>
struct _return_operator {};

template <typename T>
struct return_handler_t {
	static void main(const T &, size_t &argi) {
		fmt::println("generic return handler for {}, argi={}", 
	       		$ss_type(T).view(), argi);
	}
};

template <typename T, ThreadOutput::Properties P>
struct return_handler_t <Interpolant <T, P>> {
	static void main(const Interpolant <T, P> &interpolant, size_t &argi) {
		auto type = reconstruct_type <T> ();
		auto tout = ThreadOutput(type, argi, P);
		$tsb.context.add_thread_output(tout);

		// Fix the argument index of the original value
		auto &instr = *interpolant.ref;
		instr.template as <Intrinsic> ()
			.template as <ThreadOutput> ()
			.argi = argi;

		argi++;
	}
};

template <typename ... Args>
struct return_handler_t <std::tuple <Args...>> {
	static void main(const std::tuple <Args...> &args, size_t &argi) {
		std::apply([&](auto ... xs) {
			std::make_tuple(return_handler(xs, argi)...);
		}, args);
	}
};

template <typename T>
bool return_handler(const T &v, size_t &argi)
{
	fmt::println("ftn for {}", $ss_type(T).view());
	return_handler_t <T> ::main(v, argi);
	return true;
}

template <typename R, typename U>
requires std::is_convertible_v <U, R>
void operator<<(const _return_operator <R>, U value)
{
	// Force conversion to get expected behavior
	auto cvted = R(value);

	size_t argi = 0;
	return_handler(cvted, argi);
}

template <Stage S, typename R, typename ... Args>
struct signature {
	using args = std::tuple <std::decay_t <Args> ...>;
	using returns = R;
	using type = stage <S, R, Args...>;
};

template <Stage S, typename Rt, typename R, typename ... Args>
constexpr auto new_signature(std::function <R (Args...)>) -> signature <S, Rt, Args...>;

// TODO: custom vertex assembler stage; experiments section showing an alternative pipeline...
// TODO: ss of enums via wrapper type wrap <Enum> and then extract substring?

template <Stage S>
void inject_execution_model()
{
	if constexpr (S == Stage::RepresentationalVertex)
		$tsb.context.model = ExecutionModel::eVulkanVertex;
	else
		static_assert(false, "no execution model for stage");
}

template <typename T>
void inject_item(T &item, Reference ref)
{
	if constexpr (std::is_base_of_v <jems::handle, T>)
		item.ref = ref;
	else
		static_assert(false, ($ss("failed to inject reference into item of type ") + $ss_type(T)).view());
}

struct InjectionState {
	size_t argidx;
	size_t threadidx;

	struct Delta {
		bool argument;
		bool thread_input;
	};

	InjectionState next(Delta delta) const {
		return {
			.argidx = argidx + delta.argument,
			.threadidx = threadidx + delta.thread_input,
		};
	}
};

template <Stage S, typename T>
struct stage_argument_injector {};

// For subroutines, normals arguments are normal arguments
template <typename T>
struct stage_argument_injector <Stage::Undefined, T> {
	static auto apply(T &value, const InjectionState &state) {
		auto type = reconstruct_type <T> ();
		auto arg = Argument(type, state.argidx);
		$tsb.context.add_argument(arg);
		inject_item(value, jems::intrinsic(arg));
		return InjectionState::Delta {
			.argument = true,
			.thread_input = false,
		};
	}
};

// For vertex and fragment shaders, normal arguments are thread inputs
template <Stage S, typename T>
requires (S == Stage::RepresentationalVertex || S == Stage::RepresentationalFragment)
struct stage_argument_injector <S, T> {
	static auto apply(T &value, const InjectionState &state) {
		auto type = reconstruct_type <T> ();
		auto tin = ThreadInput(type, state.threadidx);
		$tsb.context.add_thread_input(tin);
		inject_item(value, jems::intrinsic(tin));
		return InjectionState::Delta {
			.argument = false,
			.thread_input = true,
		};
	}
};

// Always ignore the implicit context
template <Stage S, auto & ... refs>
struct stage_argument_injector <S, implicit_context <refs...>> {
	static auto apply(auto &value, const InjectionState &state) {
		return InjectionState::Delta { false, false };
	}
};

template <Stage S, size_t Index, typename ... Args>
void inject_arguments(std::tuple <Args...> &args, const InjectionState &state)
{
	auto &value = std::get <Index> (args);
	using T = std::decay_t <decltype(value)>;

	auto delta = stage_argument_injector <S, T> ::apply(value, state);
	if constexpr (Index + 1 < sizeof...(Args))
		inject_arguments <S, Index + 1> (args, state.next(delta));
}

template <Stage S, typename R, typename F>
auto compile(F ftn)
{
	using function = decltype(std::function(ftn));
	using signature = decltype(new_signature <S, R> (std::declval <function> ()));

	typename signature::type result;

	if (auto s = jems::scope(result)) {
		typename signature::args args;
		inject_execution_model <S> ();
		inject_arguments <S, 0> (args, InjectionState(0, 0));
		// TODO: need to concretize returns at the return operator...
		// (not here or at return value construction)
		std::apply(ftn, args);
	}

	return result;
}

template <Stage, int>
struct _fn_operator {};

template <Stage>
struct _stage_operator {};

#define $stage(S) _stage_operator <Stage::S> () *

#define $vertex		$stage(RepresentationalVertex)
#define $fragment	$stage(RepresentationalFragment)
#define $compute	$stage(RepresentationalCompute)

template <typename ... Args>
struct simplify_return_list {
	using type = std::tuple <Args...>;
};

template <typename T>
struct simplify_return_list <T> {
	using type = T;
};

template <>
struct simplify_return_list <> {
	using type = void;
};

#define $returns(...) decltype(fn_return_injection::Writer <decltype(_return_proxy), simplify_return_list <__VA_ARGS__> ::type> {}, void())
#define $return (_return_operator <fn_return_injection::Read <decltype(_return_proxy)> ::unfoil> ()) << 
#define $fn (_fn_operator <Stage::Undefined, __COUNTER__ + 1> ()) << [_return_proxy = fn_return_injection::proxy_tag <__COUNTER__> ()] $context_capture
#define $cafn(...) (_fn_operator <Stage::Undefined, __COUNTER__ + 1> ()) << [__VA_ARGS__ __VA_OPT__(,) _return_proxy = fn_return_injection::proxy_tag <__COUNTER__> ()] $context_capture

template <Stage S, int I>
auto operator<<(_fn_operator <S, I>, auto lambda)
{
	using R = typename fn_return_injection::Read <fn_return_injection::proxy_tag <I>> ::unfoil;
	return compile <S, R> (lambda);
}

template <Stage S, int I>
auto operator*(_stage_operator <S>, _fn_operator <Stage::Undefined, I>)
{
	return _fn_operator <S, I> ();
}
