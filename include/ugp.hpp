#pragma once

#include <functional>

#include "msv/context.hpp"
#include "msv/function_return_injection.hpp"
#include "msv/layout/layout.hpp"
#include "msv/parameter_block_filter.hpp"
#include "msv/reconstruct_type.hpp"
#include "msv/reference.hpp"
#include "msv/reflection.hpp"
#include "msv/resources.hpp"
#include "msv/return_handler.hpp"
#include "msv/stage.hpp"
#include "msv/stage_argument_injector.hpp"
#include "msv/stage_intrinsics.hpp"
#include "msv/static_string.hpp"
#include "msv/this_injection.hpp"
#include "msv/type_hash.hpp"
#include "msv/type_string_extensions.hpp"

#include "rhi/buffer.hpp"
#include "rhi/command_pool.hpp"
#include "rhi/compiler.hpp"
#include "rhi/device.hpp"
#include "rhi/group.hpp"
#include "rhi/pipelines/traditional.hpp"
#include "rhi/queue.hpp"
#include "rhi/renderpass.hpp"
#include "rhi/session.hpp"
#include "rhi/tuple_buffer.hpp"
#include "rhi/window.hpp"

#include "dsl/generators/assembly.hpp"
#include "dsl/generators/glsl.hpp"
#include "dsl/jems.hpp"
#include "dsl/tracer.hpp"

template <Stage S, typename R, typename ... Args>
struct signature {
	using args = std::tuple <std::decay_t <Args> ...>;
	using returns = R;
	using type = stage <S, R, Args...>;
};

template <Stage S, typename Rt, typename R, typename ... Args>
constexpr auto new_signature(std::function <R (Args...)>) -> signature <S, Rt, Args...>;

template <Stage S, typename R, typename F>
auto compile(F ftn)
{
	// TODO: require that all arguments are reflected?

	// TODO: custom vertex assembler stage; experiments section showing an alternative pipeline...
	// TODO: ss of enums via wrapper type wrap <Enum> and then extract substring?
	using function = decltype(std::function(ftn));
	using signature = decltype(new_signature <S, R> (std::declval <function> ()));

	typename signature::type result;

	// TODO: verify signature against provided stage...

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
