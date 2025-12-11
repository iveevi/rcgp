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
