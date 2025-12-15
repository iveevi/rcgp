#pragma once

#include "implicit_context.hpp"
#include "reference.hpp"
#include "resources.hpp"
#include "shader_stage.hpp"

template <typename T, ShaderStage ... Ss>
struct stage_wrapper {
	using stages = std::integer_sequence <ShaderStage, Ss...>;
	using type = T;

	template <ShaderStage S>
	using append_stage = std::conditional_t <
		((Ss == S) || ...),
		stage_wrapper <T, Ss...>,
		stage_wrapper <T, Ss..., S>
	>;
};

template <ShaderStage S, auto &ref, typename ... Ts>
auto add_gvr(const sequence <Ts...> &in)
{
	using base = reference <ref> ::base;
	if constexpr (!is_global_resource_v <base>) {
		return in;
	} else {
		constexpr auto exists = (std::same_as <typename Ts::type, reference <ref>> || ...);

		if constexpr (exists) {
			return sequence <
				std::conditional_t <
					std::same_as <typename Ts::type, reference <ref>>,
					typename Ts::template append_stage <S>,
					Ts
				> ...
			> ::singleton;
		} else {
			return sequence <Ts..., stage_wrapper <reference <ref>, S>> ::singleton;
		}
	}
}

template <ShaderStage S, typename ... Ts>
auto add_gvr_from_implicit_context(const sequence <Ts...> &in, const implicit_context <> &)
{
	// Empty implicit context means no additional global resources.
	return in;
}

template <ShaderStage S, typename ... Ts, auto &b, auto &...bs>
auto add_gvr_from_implicit_context(const sequence <Ts...> &in, const implicit_context <b, bs...> &)
{
	auto out = add_gvr <S, b> (in);
	if constexpr (sizeof...(bs))
		return add_gvr_from_implicit_context <S> (out, implicit_context <bs...> ());
	else
		return out;
}
