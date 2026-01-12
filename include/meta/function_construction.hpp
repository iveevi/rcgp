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
	using type = shader_stage <S, R, Args...>;
};

template <ShaderStage S, typename R, typename Rt, typename ... Args>
struct shader_signature <S, R, std::function <Rt (Args...)>>
	: shader_signature <S, R, Args...> {};

template <ShaderStage S, typename R, typename F>
auto trace(F ftn)
{
	// TODO: profile this; should be able to name blocks
	using function = decltype(std::function(ftn));
	using signature = shader_signature <S, R, function>;

	auto result = signature::type::alloc();

	// NOTE: Globally defined shaders will be traced before main()
	// so we cannot rely on the user to add the callback in time
	TimerToken::add_default_callback();
	{
		TSCOPE("JIT tracing DSL code");
		result->context.model = S;
		if (auto s = jems::scope(result)) {
			TSCOPE("primary trace");
			typename signature::args args;
			inject_arguments <S> (args);
			std::apply(ftn, args);
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

template <typename ... Args>
struct compact_returns {
	using type = std::tuple <Args...>;
};

template <typename T>
struct compact_returns <T> {
	using type = T;
};

template <>
struct compact_returns <> {
	using type = void;
};

template <typename ... Ts>
using compact_returns_t = compact_returns <Ts...> ::type;

namespace frenj_ret {

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif

template <size_t v>
struct Reader {
	friend auto adl_lever(Reader);
};

template <size_t v, typename T>
struct Writer {
	friend auto adl_lever(Reader <v>) {
		return T();
	}
};

void adl_lever();

template <size_t v>
using Read = std::remove_pointer_t <decltype(adl_lever(Reader <v> {}))>;

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

} // namespace frenj_ret

#define $returns(...) decltype(frenj_ret::Writer <__COUNTER__, compact_returns_t <__VA_ARGS__>> {}, void())
#define $return (_return_operator <frenj_ret::Read <__rpidx.value>> ()) << 
#define $fn (_fn_operator <ShaderStage::eSubroutine, __COUNTER__ + 2> ()) \
	<< [__rpidx = std::integral_constant <size_t, __COUNTER__ + 1> ()] $context_capture
#define $cafn(...) (_fn_operator <ShaderStage::eSubroutine, __COUNTER__ + 2> ()) \
	<< [__VA_ARGS__ __VA_OPT__(,) __rpidx = el <__COUNTER__ + 1> ()] $context_capture

template <ShaderStage S, int I>
auto operator<<(_fn_operator <S, I>, auto lambda)
{
	using R = frenj_ret::Read <I>;
	return trace <S, R> (lambda);
}

template <ShaderStage S, int I>
auto operator*(_stage_operator <S>, _fn_operator <ShaderStage::eSubroutine, I>)
{
	return _fn_operator <S, I> ();
}
