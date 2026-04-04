#pragma once

#include <functional>
#include <string>
#include <type_traits>

#include "../dsl/jems.hpp"
#include "contract_hell.hpp"
#include "inject_arguments.hpp"
#include "macro_hell.hpp"
#include "return_handler.hpp"
#include "shader_stage.hpp"

namespace rcgp {

template <ShaderStage S, typename R, typename ... Args>
struct shader_signature {
	using args = std::tuple <std::decay_t <Args> ...>;
	using returns = R;
	using stage = shader_stage <S, R, Args...>;
};

template <ShaderStage S, typename R, typename ... Args>
auto infer_shader_signature(std::function <R (Args...)>)
	-> shader_signature <S, R, Args...>;

// Return type inference for any callable
template <typename R, typename ... Args>
auto infer_return_type(R(*)(Args...)) -> R;

template <typename R, typename C, typename ... Args>
auto infer_return_type(R(C::*)(Args...)) -> R;

template <typename R, typename C, typename ... Args>
auto infer_return_type(R(C::*)(Args...) const) -> R;

template <typename T>
auto infer_return_type(T) -> decltype(infer_return_type(&T::operator()));

template <ShaderStage S>
// TODO: must be invocable
auto trace(auto ftn)
{
	using F = decltype(infer_shader_signature <S> (std::function(ftn)));

	Tracer::singleton.type_cache.clear();

	auto result = F::stage::alloc();

	result->stage = S;
	if (auto s = jems::scope(result)) {
		typename F::args args;
		inject_arguments <S> (args);
		
		// TODO: check void...
		if constexpr (std::is_same_v <typename F::returns, void>) {
			std::apply(ftn, args);
		} else {
			auto returns = std::apply(ftn, args);
			// TODO: provide stage
			size_t argi = 0;
			return_handler <S> (returns, argi);
		}
	}

	return result;
}

template <ShaderStage S>
struct _fn_tag {
	std::string name;
};

template <ShaderStage S>
auto operator<<(_fn_tag <S> tag, auto lambda)
{
	auto result = trace <S> (lambda);
	result->name = tag.name;
	return result;
}

// TODO: tests for this...
template <bool B, typename T>
using enable_if = std::conditional_t <B, T, jems::null>;

#define rcgp_vertex	(_fn_tag <ShaderStage::eVertex> ("main"))
#define rcgp_fragment	(_fn_tag <ShaderStage::eFragment> ("main"))
#define rcgp_compute	(_fn_tag <ShaderStage::eCompute> ("main"))
#define rcgp_mesh	(_fn_tag <ShaderStage::eMesh> ("main"))
#define rcgp_task	(_fn_tag <ShaderStage::eTask> ("main"))
#define rcgp_raygen	(_fn_tag <ShaderStage::eRayGeneration> ("main"))
#define rcgp_chit	(_fn_tag <ShaderStage::eClosestHit> ("main"))
#define rcgp_miss	(_fn_tag <ShaderStage::eMiss> ("main"))

#define rcgp_context(...)	_ctx = icontext_from_vptr((void (*)(__VA_ARGS__)) nullptr)] (__VA_ARGS__)

#define $def static inline

#define $shader(type, ...)	rcgp_##type << [__VA_ARGS__ __VA_OPT__(,) rcgp_context
#define $subroutine(name, ...)	(_fn_tag <ShaderStage::eSubroutine> (#name)) << [__VA_ARGS__ __VA_OPT__(,) rcgp_context
#define $decl(name) auto name = $subroutine(name)

#define $enable_if(cond, arg) (ENABLE_IF, cond, arg)

#define ICONTEXT_GENERATOR(ctx, ftn) , typename decltype(ftn)::icontext
#define $inherits(...) jems::null MAP(ICONTEXT_GENERATOR, /* N/A */, __VA_ARGS__)

#define $shader_t(S, R, ...) decltype($shader(S)(__VA_ARGS__) { return R(); })
#define $subroutine_t(S, R, ...) decltype($subroutine(_tmp)(__VA_ARGS__) { return R(); })

#define $contracts(...) jems::null MAP(CONTRACT_GENERATOR, /* N/A */, __VA_ARGS__)

#define $return_t(ftn) decltype(rcgp::infer_return_type(ftn))

} // namespace rcgp
