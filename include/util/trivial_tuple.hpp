#pragma once

#include <cstddef>

// Trivially constructable tuple that supports POD fields and dynamic arrays
template <typename ... Ts>
class trivial_tuple {};

template <>
class trivial_tuple <> {
	template <size_t Index>
	static consteval size_t offset() {
		static_assert(Index != Index, "tuple index out of range");
		return 0;
	}
};

template <typename T, typename ... Ts>
class trivial_tuple <T, Ts...> {
	T x;
	[[no_unique_address]] trivial_tuple <Ts...> rest;
public:
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
};
