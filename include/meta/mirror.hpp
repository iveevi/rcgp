#pragma once

#include "std430.hpp"
#include "reflection.hpp"
#include "expand_reflection.hpp"
#include "static_string.hpp"
#include "../util/cti.hpp"

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
	static_error("resource translator not implemented for type "_ss + $ss_type(T));
	using type = std::nullptr_t;
	using value_type = std::nullptr_t;
	using elment_type = std::nullptr_t;
};

template <typename T>
using ResourceType = resource_translator <T> ::type;

template <typename T>
using DataType = decltype([] {
	using RMT = resource_translator <T> ::value_type;
	static_assert(not std::same_as <RMT, std::nullptr_t>,
	       ("no canonical type for resource of type "_ss + $ss_type(T)).view());
	return RMT();
} ());

template <typename T>
using DynamicElementType = decltype([] {
	using RME = resource_translator <T> ::element_type;
	static_assert(not std::same_as <RME, std::nullptr_t>,
	       ("no dynamic element for resource of type "_ss + $ss_type(T)).view());
	return RME();
} ());
