#pragma once

#include "layout/all.hpp"
#include "reflection.hpp"
#include "expand_reflection.hpp"

// Type mirrors
template <reflected T, template <typename> typename L = layouts::std430>
using TypeMirror = decltype([] {
	using reflection = expand_reflection_t <T>;
	using layout = L <reflection>;
	using hint = layout::hint;
	using type = scaffold_lookup <hint, T, true> ::type;
	return type();
} ());

// Resource mirrors
template <typename T>
struct resource_translator {
	static_assert(false, ($ss("resource translator not implemented for type ") + $ss_type(T)).view());
	using type = std::nullptr_t;
	using value_type = std::nullptr_t;
	using elment_type = std::nullptr_t;
};

template <typename T>
using ResourceMirror = resource_translator <T> ::type;

template <typename T>
using DataType = decltype([] {
	using RMT = resource_translator <T> ::value_type;
	static_assert(not std::same_as <RMT, std::nullptr_t>,
	       ($ss("no canonical type for resource of type ") + $ss_type(T)).view());
	return RMT();
} ());

template <typename T>
using DynamicDataType = decltype([] {
	using RME = resource_translator <T> ::element_type;
	static_assert(not std::same_as <RME, std::nullptr_t>,
	       ($ss("no dynamic element for resource of type ") + $ss_type(T)).view());
	return RME();
} ());
