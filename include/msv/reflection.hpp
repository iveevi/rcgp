#pragma once

#include <cstdlib>

#include "../util/meta.hpp"
#include "field_name_injection.hpp"
#include "macro_hell.hpp"
#include "this_injection.hpp"

// Reflection types
template <typename Original, typename ... Ts>
struct aggregate_reflection {};

template <typename Original, typename ... Ts>
auto new_aggregate_reflection(sequence <Ts...>)
	-> aggregate_reflection <Original, Ts...>;

template <typename T>
struct parameter_block_reflection {};

template <typename T>
struct structured_buffer_reflection {};

template <typename T, size_t D>
struct sampler_reflection {};

template <auto &ref, typename T>
struct reference_reflection {};

template <typename R, typename ... Args>
struct function_reflection {};

// TODO: stage specific reflections... just one parameter for classification

// Querying reflection status
template <typename T>
constexpr bool has_reflection()
{
	if constexpr (requires { T::_ugp_has_reflection; })
		return T::_ugp_has_reflection;
	else
		return false;
}

// Type registry for wholistic reflection
template <typename T, std::size_t Index>
struct scaffold_field {
	using value_type = T;
	static constexpr auto index = Index;
};

// TODO: also collect field names as static_strings
#define AGGREGATE_FIELD_GENERATOR(T, field) std::declval <decltype(field)> (),
#define AGGREGATE_SCAFFOLD_GENERATOR(T, field)	\
	scaffold_field <decltype(T::field), (__COUNTER__ - counter_base - 1)> field;

#define UGP_REFLECTION_STAMP static constexpr bool _ugp_has_reflection = true

#define $reflection(...)							\
	THIS_INJECTION();							\
	using reflection = decltype(new_aggregate_reflection <This> (		\
		sequence {							\
			MAP(AGGREGATE_FIELD_GENERATOR, /* NA */ , __VA_ARGS__)	\
		}								\
	));									\
	struct scaffold {							\
		static constexpr size_t counter_base = __COUNTER__;		\
		MAP(AGGREGATE_SCAFFOLD_GENERATOR, This, __VA_ARGS__)		\
	};									\
	MAP(FIELD_NAME_INJECTION, This, __VA_ARGS__)				\
	UGP_REFLECTION_STAMP;
