#pragma once

#include <cstdint>
#include <type_traits>

namespace rcgp {

// Classifying host primitive types
template <typename T>
concept native_scalar = bool(false
	| std::is_same_v <T, bool>
	| std::is_same_v <T, uint32_t>
	| std::is_same_v <T, int32_t>
	| std::is_same_v <T, float>
	| std::is_same_v <T, uint64_t>);

template <typename T>
concept native_int_scalar = bool(false
	| std::is_same_v <T, uint32_t>
	| std::is_same_v <T, int32_t>);

template <typename T>
concept native_float_scalar = bool(false
	| std::is_same_v <T, float>);

} // namespace rcgp
