#pragma once

#include <type_traits>

#include "primitives_concepts.hpp"
#include "scalar.hpp"

template <typename T>
struct is_scalar : std::false_type {};

template <native_scalar T>
struct is_scalar <scalar <T>> : std::true_type {};

template <typename T>
constexpr bool is_scalar_v = is_scalar <T> ::value;

template <typename T>
struct projection {
	using type = T;
};

template <native_scalar T>
struct projection <T> {
	using type = scalar <T>;
};

template <typename T>
using projection_t = projection <T> ::type;

template <typename T>
auto project(const T &value)
{
	return projection_t <T> (value);
}

template <typename T, typename U>
concept projectively_equivalent = std::same_as <
	projection_t <T>,
	projection_t <U>
>;

template <typename T>
concept projectively_scalar = is_scalar_v <projection_t <T>>;

template <typename T>
concept projectively_int_scalar = projectively_scalar <T>
	&& native_int_scalar <
		typename projection_t <T> ::native_scalar_type
	>;
