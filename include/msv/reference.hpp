#pragma once

#include <type_traits>

#include "reflection.hpp"
#include "static_string.hpp"

template <auto &resource>
using reference_base = std::remove_reference_t <decltype(resource)>;

template <auto &rsrc>
struct reference : reference_base <rsrc> {
	using value_type = reference_base <rsrc>;

	static_assert(has_reflection <value_type> (),
	       // TODO: string only format style...
	       ($ss_type(value_type) + $ss(
	       " has no valid reflection, perhaps you forgot to"
	       " add it to the registry via $reflection(...)?")).view());

	static constexpr auto &handle = rsrc;

	using reflection = reference_reflection <
		rsrc,
		typename value_type::reflection
	>;
};

template <typename T>
struct is_reference : std::false_type {};

template <auto &rsrc>
struct is_reference <reference <rsrc>> : std::true_type {};

#define $use(name)	reference <name> name

// TODO: for comparisons just use std::same_as...
