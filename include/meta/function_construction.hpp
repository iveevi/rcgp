#pragma once

#include <functional>

#include "../dsl/jems.hpp"
#include "function_return_injection.hpp"
#include "injection_state.hpp"
#include "shader_stage.hpp"

template <ShaderStage S, typename R, typename ... Args>
struct signature {
	using args = std::tuple <std::decay_t <Args> ...>;
	using returns = R;
	using type = shader_stage <S, R, Args...>;
};

template <ShaderStage S, typename Rt, typename R, typename ... Args>
constexpr auto new_signature(std::function <R (Args...)>) -> signature <S, Rt, Args...>;

template <ShaderStage S, typename R, typename F>
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

template <ShaderStage, int>
struct _fn_operator {};

template <ShaderStage>
struct _stage_operator {};

#define $stage(S) _stage_operator <ShaderStage::S> () *

#define $vertex		$stage(Vertex)
#define $fragment	$stage(Fragment)
#define $compute	$stage(Compute)

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
#define $fn (_fn_operator <ShaderStage::Undefined, __COUNTER__ + 1> ()) << [_return_proxy = fn_return_injection::proxy_tag <__COUNTER__> ()] $context_capture
#define $cafn(...) (_fn_operator <ShaderStage::Undefined, __COUNTER__ + 1> ()) << [__VA_ARGS__ __VA_OPT__(,) _return_proxy = fn_return_injection::proxy_tag <__COUNTER__> ()] $context_capture

template <ShaderStage S, int I>
auto operator<<(_fn_operator <S, I>, auto lambda)
{
	using R = typename fn_return_injection::Read <fn_return_injection::proxy_tag <I>> ::unfoil;
	return compile <S, R> (lambda);
}

template <ShaderStage S, int I>
auto operator*(_stage_operator <S>, _fn_operator <ShaderStage::Undefined, I>)
{
	return _fn_operator <S, I> ();
}
