#pragma once

#include "reflection.hpp"

template <typename T, size_t ... Is>
struct field_trace {
	using value_type = T;
};

// Access chain through static indices
template <typename T, size_t I, size_t ... Is>
struct static_access_chain_handler {
	static auto &main(T &value) {
		static_assert(false, ($ss("static access chain not implemented for ") + $ss_type(T)).view());
		return value;
	}
};

// Aggregate already have a method for this, just recurse over that
template <aggregate T, size_t I, size_t ... Is>
struct static_access_chain_handler <T, I, Is...> {
	static auto &main(T &value) {
		auto &one = value.template _ugp_field_reference <I> ();

		using U = std::remove_reference_t <decltype(one)>;
		if constexpr (sizeof...(Is))
			return static_access_chain_handler <U, Is...> ::main(one);
		else
			return one;
	}
};

template <size_t I, size_t ... Is, reflected T>
auto &static_access_chain(T &value)
{
	return static_access_chain_handler <T, I, Is...> ::main(value);
}
