#pragma once

#include <array>
#include <cstddef>
#include <vector>

// Trivially constructable tuple that supports POD fields and dynamic arrays
template <typename ... Ts>
struct trivial_tuple {};

template <>
struct trivial_tuple <> {
	template <size_t Index>
	static consteval size_t offset() {
		static_assert(Index != Index, "tuple index out of range");
		return 0;
	}
};

template <typename T, typename ... Ts>
struct trivial_tuple <T, Ts...> {
	// TODO: private...
	T x;
	[[no_unique_address]] trivial_tuple <Ts...> rest;

	template <size_t Index>
	auto &get() {
		if constexpr (Index == 0) {
			return x;
		} else {
			return rest.template get <Index - 1> ();
		}
	}

	template <size_t Index>
	const auto &get() const {
		if constexpr (Index == 0) {
			return x;
		} else {
			return rest.template get <Index - 1> ();
		}
	}

	template <size_t Index>
	static consteval size_t offset() {
		if constexpr (Index == 0) {
			return 0;
		} else {
			return sizeof(T) + trivial_tuple <Ts...> ::template offset <Index - 1> ();
		}
	}

	static constexpr bool dynamic = [] constexpr {
		if constexpr (sizeof...(Ts))
			return trivial_tuple <Ts...> ::dynamic;
		else
			return false;
	} ();
};
