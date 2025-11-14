#pragma once

#include <cstdint>

#include "reflection.hpp"
#include "static_string.hpp"
#include "type_hash.hpp"

// TODO: Reference counter

// Helpers for stringifying reflection data
template <size_t Indentation, bool Newline, size_t I, typename T, typename ... Args>
consteval auto enumerated_type_strings()
{
	constexpr auto main = static_string <Indentation> ('\t')
		+ $ss_ulong_decimal(I) + $ss(": ") + $ss_type_indented(T, Indentation)
		+ static_string <Newline> ('\n');

	if constexpr (sizeof...(Args)) {
		constexpr auto rest = enumerated_type_strings <
			Indentation,
			Newline,
			I + 1,
			Args...
		> ();

		return main + rest;
	} else {
		return main;
	}
}

template <size_t Indentation, bool Newline, size_t I, typename Original, typename T, typename ... Args>
consteval auto enumerated_field_strings()
{
	constexpr auto main = static_string <Indentation> ('\t')
		+ $field_name(Original, I) + $ss(": ") + $ss_type_indented(T, Indentation)
		+ static_string <Newline> ('\n');

	if constexpr (sizeof...(Args)) {
		constexpr auto rest = enumerated_field_strings <
			Indentation,
			Newline,
			I + 1,
			Original,
			Args...
		> ();

		return main + rest;
	} else {
		return main;
	}
}

// Overload name generator for reflection types
template <typename Original, typename ... Args>
struct type_string <aggregate_reflection <Original, Args...>> {
	template <size_t I>
	static consteval auto eval() {
		// TODO: reconstruct the type name of this aggregate...
		constexpr auto tabs = static_string <I> ('\t');
		return $ss_type_indented(Original, I) + $ss(" {\n")
			+ enumerated_field_strings <I + 1, true, 0, Original, Args...> ()
			+ tabs + $ss("}");
	}
};

template <typename T>
struct type_string <parameter_block_reflection <T>> {
	template <size_t I>
	static consteval auto eval() {
		return $ss("parameter block of ") + $ss_type_indented(T, I);
	}
};

template <auto &ref, typename T>
struct type_string <reference_reflection <ref, T>> {
	struct inner {};

	template <size_t I>
	static consteval auto eval() {
		constexpr size_t id = 0;
		return $ss("reference (hash: ")
			+ $ss_ulong_hex(type_hash_v <inner>)
			+ $ss(") to ")
			+ $ss_type_indented(T, I);
	}
};

template <typename R, typename ... Args>
struct type_string <function_reflection <R, Args...>> {
	template <size_t I>
	static consteval auto eval() {
		constexpr auto tabs = static_string <I> ('\t');
		return $ss("function {\n")
			+ tabs + $ss("\treturn: ") + $ss_type_indented(R, I + 1) + $ss("\n")
			+ tabs + $ss("\targs: {\n")
			+ enumerated_type_strings <I + 2, true, 0, Args...> ()
			+ tabs + $ss("\t}\n")
			+ tabs + $ss("}");
	}
};
