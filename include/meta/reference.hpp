#pragma once

#include <type_traits>

#include "reflection.hpp"
#include "static_string.hpp"
#include "static_access_chain.hpp"

template <auto &rsrc>
using reference_base_t = std::decay_t <decltype(rsrc)>;

// TODO: should enforce that raw_base is a resource type
template <auto &rsrc>
struct reference : reference_base_t <rsrc> {
	using base = reference_base_t <rsrc>;
	static_assert(has_reflection <base> ());
	using reflection = reference_reflection <rsrc, typename base::reflection>;
};

template <typename T>
struct is_reference : std::false_type {};

template <auto &rsrc>
struct is_reference <reference <rsrc>> : std::true_type {};

#define $use(name)	reference <name> name

// References to parameter blocks need to handled in one big step
template <auto &rsrc, size_t I, size_t ... Is>
struct static_access_chain_handler <reference <rsrc>, I, Is...> {
	static auto &main(reference <rsrc> &value) {
		using T = reference_base_t <rsrc>;
		T &cvted = static_cast <T &> (value);
		return static_access_chain_handler <T, I, Is...> ::main(value);
	}
};
