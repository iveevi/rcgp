#pragma once

#include "../util/cti.hpp"

namespace rcgp {

template <auto &ref>
using reference_base_t = std::decay_t <decltype(ref)>;

// TODO: refactor to contract!
// TODO: should enforce that base is a resource type
template <auto &ref>
struct reference : reference_base_t <ref> {
	static inline auto address = &ref;
	static constexpr auto &handle = ref;
};

TYPE_TRAIT(is_reference);
	template <auto &ref>
	TYPE_TRAIT_INCLUDES(is_reference, reference <ref>);

} // namespace rcgp
