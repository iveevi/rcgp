#pragma once

#include <cstdlib>

#include "macro_hell.hpp"
#include "this_injection.hpp"
#include "field_name_injection.hpp"
#include "meta.hpp"

// Reflection types
struct nil_reflection {};

template <typename T, size_t I>
struct field_reflection {};

template <typename Original, typename ... Args>
struct aggregate_reflection {};

template <typename Original, typename ... Args>
auto new_aggregate_reflection(sequence <Args...>)
	-> aggregate_reflection <Original, Args...>;

template <typename T>
struct parameter_block_reflection {};

template <auto &ref, typename T>
struct reference_reflection {};

template <typename R, typename ... Args>
struct function_reflection {};

// TODO: stage specific reflections... just one parameter for classification

// Querying reflection status
template <typename T>
concept reflection_holder = requires { typename T::reflection; };

template <typename T>
struct reflection_expander : std::false_type {
	// TODO: refactor to expanded
	using type = T;
};

template <reflection_holder T>
struct reflection_expander <T> : std::true_type {
	using type = typename T::reflection;
};

template <typename T>
constexpr bool has_reflection = reflection_expander <T> ::value;

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
	MAP(FIELD_NAME_INJECTION, This, __VA_ARGS__)
