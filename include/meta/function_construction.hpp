#pragma once

#include <functional>

#include "../dsl/jems.hpp"
#include "../dsl/optimizer.hpp"
#include "../util/cti.hpp"
#include "../util/timer.hpp"
#include "inject_arguments.hpp"
#include "shader_stage.hpp"

template <ShaderStage S, typename R, typename ... Args>
struct shader_signature {
	using args = std::tuple <std::decay_t <Args> ...>;
	using returns = R;
	using stage = shader_stage <S, R, Args...>;
};

template <ShaderStage S, typename R, typename ... Args>
auto infer_shader_signature(std::function <R (Args...)>)
	-> shader_signature <S, R, Args...>;

template <ShaderStage S>
// TODO: must be invocable
auto trace(auto ftn)
{
	using F = decltype(infer_shader_signature <S> (std::function(ftn)));

	auto result = F::stage::alloc();

	// NOTE: Globally defined shaders will be traced before main()
	// so we cannot rely on the user to add the callback in time
	TimerToken::add_default_callback();
	{
		TSCOPE("JIT tracing DSL code");
		result->context.model = S;
		Tracer::singleton.type_cache.clear();
		if (auto s = jems::scope(result)) {
			TSCOPE("primary trace");
			typename F::args args;
			inject_arguments <S> (args);
			
			// TODO: check void...
			if constexpr (std::is_same_v <typename F::returns, void>) {
				std::apply(ftn, args);
			} else {
				auto returns = std::apply(ftn, args);
				// TODO: provide stage
				size_t argi = 0;
				return_handler(returns, argi);
			}
		}

		optimize_block(result);
	}
	TimerToken::remove_callback("default");

	return result;
}

template <ShaderStage, int>
struct _fn_operator {};

template <ShaderStage>
struct _stage_operator {};

#define $stage(S) _stage_operator <ShaderStage::S> () *

#define $vertex		$stage(eVertex)
#define $fragment	$stage(eFragment)
#define $compute	$stage(eCompute)
#define $task		$stage(eTask)
#define $mesh		$stage(eMesh)

#define $fn (_fn_operator <ShaderStage::eSubroutine, __COUNTER__ + 2> ()) \
	<< [] $context_capture
#define $cafn(...) (_fn_operator <ShaderStage::eSubroutine, __COUNTER__ + 2> ()) \
	<< [__VA_ARGS__ __VA_OPT__(,)] $context_capture

template <ShaderStage S, int I>
auto operator<<(_fn_operator <S, I>, auto lambda)
{
	return trace <S> (lambda);
}

template <ShaderStage S, int I>
auto operator*(_stage_operator <S>, _fn_operator <ShaderStage::eSubroutine, I>)
{
	return _fn_operator <S, I> ();
}
