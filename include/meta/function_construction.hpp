#pragma once

#include <functional>

#include "../dsl/jems.hpp"
#include "../dsl/optimizer.hpp"
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

template <ShaderStage S>
struct _fn_tag {
	std::string name;
};

template <ShaderStage S>
auto operator<<(_fn_tag <S>, auto lambda)
{
	return trace <S> (lambda);
}

#define rcgp_vertex	(_fn_tag <ShaderStage::eVertex> ("main"))
#define rcgp_fragment	(_fn_tag <ShaderStage::eFragment> ("main"))
#define rcgp_compute	(_fn_tag <ShaderStage::eCompute> ("main"))
#define rcgp_mesh	(_fn_tag <ShaderStage::eMesh> ("main"))
#define rcgp_task	(_fn_tag <ShaderStage::eTask> ("main"))

#define $shader(type, ...)	rcgp_##type << [__VA_ARGS__] rcgp_build_context
#define $subroutine(name, ...)	(_fn_tag <ShaderStage::eSubroutine> (#name)) << [__VA_ARGS__] rcgp_build_context

#define REFERENCE_GENERATOR(ctx, name) , reference <name> name
#define $contracts(...) std::nullptr_t MAP(REFERENCE_GENERATOR, /* N/A */, __VA_ARGS__)
