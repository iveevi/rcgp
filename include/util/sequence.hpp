#pragma once

#include <tuple>

// Tuples without the baggage
template <typename ... Args>
struct sequence {
	static constexpr size_t size = sizeof...(Args);

	sequence(Args...) requires (size > 0) {}
	sequence(std::type_identity <Args> ...) {}

	template <typename T>
	using push_front_t = sequence <T, Args...>;

	template <typename T>
	using push_back_t = sequence <Args..., T>;

	using tuple = std::tuple <Args...>;

	template <template <typename ...> typename F>
	using invoke = F <Args...>;

	static inline sequence singleton {
		std::type_identity <Args> ()...
	};
};

// Static enumeration
template <size_t I>
using el = std::integral_constant <size_t, I>;

template <size_t ... Is>
auto el_series(std::index_sequence <Is...>)
{
	return std::tuple(el <Is> ()...);
}
