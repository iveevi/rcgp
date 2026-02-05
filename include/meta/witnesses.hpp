#pragma once

#include "contract.hpp"
#include "static_string.hpp"
#include "resource_translations.hpp"

namespace rcgp {

// Type mirror and witness
template <typename T>
using DataType = decltype([] {
	using RMT = resource_translation <T> ::host_type;
	static_assert(not std::same_as <RMT, std::nullptr_t>,
	       ("no canonical type for resource of type "_ss + $ss_type(T)).view());
	return RMT();
} ());

template <auto &ref>
using DataTypeFor = DataType <reference_base_of <ref>>;

// Resource mirror and witness
template <typename T>
using ResourceType = resource_translation <T> ::handle_type;

template <auto &ref>
using ResourceTypeFor = ResourceType <reference_base_of <ref>>;

} // namespace rcgp
