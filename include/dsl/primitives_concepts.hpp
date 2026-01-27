#pragma once

#include <cstdint>

namespace rcgp {

// Classifying host primitive types
template <typename T>
concept native_scalar = bool(false
	| std::same_as <T, bool>
	| std::same_as <T, uint32_t>
	| std::same_as <T, int32_t>
	| std::same_as <T, float>
	| std::same_as <T, uint64_t>);

template <typename T>
concept native_int_scalar = bool(false
	| std::same_as <T, uint32_t>
	| std::same_as <T, int32_t>);

template <typename T>
concept native_float_scalar = bool(false
	| std::same_as <T, float>);

} // namespace rcgp
