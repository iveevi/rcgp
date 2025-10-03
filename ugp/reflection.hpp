#pragma once

#include <cstdlib>

#include "macro_hell.hpp"
#include "this_injection.hpp"
#include "field_name_injection.hpp"

template <typename ... Args>
struct sequence_t {
	sequence_t(Args ...) {}
};

// Reflection types
struct nil_reflection_t {};

template <typename T, size_t I>
struct field_reflection_t {};

template <typename Original, typename ... Args>
struct aggregate_reflection_t {};

template <typename Original, typename ... Args>
auto aggregate_reflection_generator(sequence_t <Args...>)
	-> aggregate_reflection_t <Original, Args...>;

template <typename T>
struct parameter_block_reflection_t {};

template <auto &ref, typename T>
struct referencing_reflection_t {};

template <typename R, typename ... Args>
struct function_reflection_t {};

// TODO: stage specific reflections... just one parameter for classification

template <typename T>
concept reflection_holder = requires { typename T::reflection; };

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
	using reflection = decltype(aggregate_reflection_generator <This> (	\
		sequence_t {							\
			MAP(AGGREGATE_FIELD_GENERATOR, /* NA */ , __VA_ARGS__)	\
		}								\
	));									\
	struct scaffold {							\
		static constexpr size_t counter_base = __COUNTER__;		\
		MAP(AGGREGATE_SCAFFOLD_GENERATOR, This, __VA_ARGS__)		\
	};									\
	MAP(FIELD_NAME_INJECTION, This, __VA_ARGS__)
