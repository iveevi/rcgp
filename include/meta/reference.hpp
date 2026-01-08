#pragma once

#include <type_traits>

#include "reflection.hpp"
#include "static_string.hpp"
#include "field_access.hpp"

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

template <typename T>
constexpr bool is_reference_v = is_reference <T> ::value;

#define $ref(name)		reference <name> name
#define $tref(name, ...)	reference <name <__VA_ARGS__>> name

// Accessing through a reference forwards to its underlying resource
template <size_t I, size_t ... Is, auto &rsrc>
auto &field_access(reference <rsrc> &value)
{
	using T = reference_base_t <rsrc>;
	T &base = static_cast <T &> (value);
	return field_access <I, Is...> (base);
}
