#pragma once

#include <tuple>

// Tuples without the baggage
template <typename ... Args>
struct sequence {
	static constexpr size_t size = sizeof...(Args);

	constexpr sequence(std::type_identity <Args> ...) {}

	template <size_t I>
	using get = decltype([] {
		// GCC doesn't like indexing empty packs :(
		if constexpr (I < sizeof...(Args))
			return (Args...[I]) {};
		else
			return int();
	} ());

	template <typename T>
	using push_front_t = sequence <T, Args...>;

	template <typename T>
	using push_back_t = sequence <Args..., T>;

	template <template <typename ...> typename F>
	using invoke = F <Args...>;

	static constexpr auto iseq() {
		return std::make_index_sequence <size> ();
	}

	static constexpr auto reify() {
		return sequence {
			std::type_identity <Args> {}...
		};
	}
};

// Static enumeration
template <size_t I>
using el = std::integral_constant <size_t, I>;

template <size_t ... Is>
auto el_series(std::index_sequence <Is...>)
{
	return std::tuple(el <Is> ()...);
}
