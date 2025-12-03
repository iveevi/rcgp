#pragma once

#include <type_traits>

#include "reflection.hpp"
#include "static_string.hpp"
#include "static_access_chain.hpp"

template <auto &rsrc>
struct reference_base {
	using type = std::remove_reference_t <decltype(rsrc)>;
};

template <auto &rsrc>
using reference_base_t = reference_base <rsrc> ::type;

template <auto &rsrc>
struct reference : reference_base_t <rsrc> {
	using raw_base = std::remove_reference_t <decltype(rsrc)>;

	static_assert(has_reflection <raw_base> (),
	       // TODO: string only format style...
	       ($ss_type(raw_base) + $ss(
	       " has no valid reflection, perhaps you forgot to"
	       " add it to the registry via $reflection(...)?")).view());

	static constexpr auto &handle = rsrc;

	using reflection = reference_reflection <
		rsrc,
		// We still need the full information in
		// cases of ParameterBlock, StorageBuffer, etc.
		typename raw_base::reflection
	>;
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
