#pragma once

#include <tuple>

// Tuples without the backage
template <typename ... Args>
struct sequence {
	sequence(Args ...) {}

	template <typename T>
	using append_t = sequence <Args..., T>;

	using tuple = std::tuple <Args...>;
};

// Static enumeration
template <size_t I>
using el = std::integral_constant <size_t, I>;

template <size_t ... Is>
auto el_series(std::index_sequence <Is...>)
{
	return std::tuple(el <Is> ()...);
}
