#pragma once

#include "../util/cti.hpp"
#include "reflection.hpp"
// #include "resources.hpp"

template <auto &ref>
using reference_base_t = std::decay_t <decltype(ref)>;

// TODO: should enforce that base is a resource type
template <auto &ref>
struct reference : reference_base_t <ref> {
	static inline auto address = &ref;
	static constexpr auto &handle = ref;

	using reflection = reference_reflection <
		ref, typename reference_base_t <ref> ::reflection
	>;
};

TYPE_TRAIT(is_reference);
	template <auto &ref>
	TYPE_TRAIT_INCLUDES(is_reference, reference <ref>);
