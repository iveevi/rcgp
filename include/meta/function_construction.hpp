#pragma once

#include <functional>
#include <type_traits>

#include "../dsl/jems.hpp"
#include "../dsl/optimizer.hpp"
#include "../util/timer.hpp"
#include "inject_arguments.hpp"
#include "macro_hell.hpp"
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

template <ShaderStage S>
struct _fn_tag {
	std::string name;
};

template <ShaderStage S>
auto operator<<(_fn_tag <S>, auto lambda)
{
	return trace <S> (lambda);
}

template <bool B, typename T>
using enable_if = std::conditional_t <B, T, std::nullptr_t>;

#define rcgp_vertex	(_fn_tag <ShaderStage::eVertex> ("main"))
#define rcgp_fragment	(_fn_tag <ShaderStage::eFragment> ("main"))
#define rcgp_compute	(_fn_tag <ShaderStage::eCompute> ("main"))
#define rcgp_mesh	(_fn_tag <ShaderStage::eMesh> ("main"))
#define rcgp_task	(_fn_tag <ShaderStage::eTask> ("main"))

#define rcgp_context(...)	_ctx = icontext_from_vptr((void (*)(__VA_ARGS__)) nullptr)] (__VA_ARGS__)

#define $def static inline

#define $shader(type, ...)	rcgp_##type << [__VA_ARGS__ __VA_OPT__(,) rcgp_context
#define $subroutine(name, ...)	(_fn_tag <ShaderStage::eSubroutine> (#name)) << [__VA_ARGS__ __VA_OPT__(,) rcgp_context

#define $enable_if(cond, arg) (ENABLE_IF, cond, arg)

#define ICONTEXT_GENERATOR(ctx, ftn) , typename decltype(ftn)::icontext
#define $inherits(...) std::nullptr_t MAP(ICONTEXT_GENERATOR, /* N/A */, __VA_ARGS__)

// TODO: move to separate header... contract_hell
#define RCGP_REFERENCE_FROM_NAME(name) , reference <name> name
#define RCGP_REFERENCE_FROM_TUPLE(name, ref) , reference <(ref)> name

#define RCGP_CONTRACT_IS_ENABLE_IF(tag, ...) \
	MACRO_CHECK(MACRO_CAT(RCGP_CONTRACT_IS_ENABLE_IF_PROBE_, tag))
#define RCGP_CONTRACT_IS_ENABLE_IF_PROBE_ENABLE_IF MACRO_PROBE()

#define RCGP_CONTRACT_ENABLE_IF_TUPLE(cond, name, ref) \
	, std::conditional_t <cond, reference <(ref)>, std::nullptr_t> name
#define RCGP_CONTRACT_ENABLE_IF_NAME(cond, name) \
	, std::conditional_t <cond, reference <name>, std::nullptr_t> name

#define RCGP_CONTRACT_FROM_ENABLE_IF(tag, cond, arg) \
	RCGP_CONTRACT_FROM_ENABLE_IF_ARG(cond, arg)
#define RCGP_CONTRACT_ENABLE_IF_TUPLE_WRAPPER(cond, arg) \
	MACRO_APPLY(RCGP_CONTRACT_ENABLE_IF_TUPLE, (cond, MACRO_UNPACK arg))
#define RCGP_CONTRACT_ENABLE_IF_NAME_WRAPPER(cond, arg) \
	RCGP_CONTRACT_ENABLE_IF_NAME(cond, arg)
#define RCGP_CONTRACT_FROM_ENABLE_IF_ARG(cond, arg) \
	MACRO_IF(MACRO_IS_PAREN(arg))	\
	(	\
		RCGP_CONTRACT_ENABLE_IF_TUPLE_WRAPPER,	\
		RCGP_CONTRACT_ENABLE_IF_NAME_WRAPPER	\
	)(cond, arg)

#define RCGP_CONTRACT_FROM_ENABLE_IF_WRAPPER(...) \
	RCGP_CONTRACT_FROM_ENABLE_IF(__VA_ARGS__)
#define RCGP_REFERENCE_FROM_TUPLE_WRAPPER(...) \
	RCGP_REFERENCE_FROM_TUPLE(__VA_ARGS__)
#define RCGP_REFERENCE_FROM_PAREN(...) \
	MACRO_IF(RCGP_CONTRACT_IS_ENABLE_IF(__VA_ARGS__)) \
	(	\
		RCGP_CONTRACT_FROM_ENABLE_IF_WRAPPER, \
		RCGP_REFERENCE_FROM_TUPLE_WRAPPER \
	)(__VA_ARGS__)

#define RCGP_REFERENCE_FROM_ARG(arg) \
	MACRO_IF(MACRO_IS_PAREN(arg))(RCGP_REFERENCE_FROM_PAREN arg, RCGP_REFERENCE_FROM_NAME(arg))
#define REFERENCE_GENERATOR(ctx, arg) RCGP_REFERENCE_FROM_ARG(arg)

#define $contracts(...) std::nullptr_t MAP(REFERENCE_GENERATOR, /* N/A */, __VA_ARGS__)
