#pragma once

#include "reflection.hpp"

template <typename T>
struct expand_reflection {
	using type = T;
};

template <typename T>
using expand_reflection_t = typename expand_reflection <T> ::type;

template <reflected T>
struct expand_reflection <T> {
	using type = expand_reflection_t <typename T::reflection>;
};

template <typename T, int64_t N>
struct expand_reflection <array_reflection <T, N>> {
	using type = array_reflection <expand_reflection_t <T>, N>;
};

template <typename T>
struct expand_reflection <resource_group_reflection <T>> {
	using type = resource_group_reflection <expand_reflection_t <T>>;
};

template <typename Original, typename ... Ts>
struct expand_reflection <aggregate_reflection <Original, Ts...>> {
	using type = aggregate_reflection <
		Original,
		expand_reflection_t <Ts>...
	>;
};

template <reflected T>
constexpr bool is_dynamic_v = is_dynamic_reflection <expand_reflection_t <T>> ::value;

template <reflected T>
constexpr bool is_static_v = !is_dynamic_reflection <expand_reflection_t <T>> ::value;
