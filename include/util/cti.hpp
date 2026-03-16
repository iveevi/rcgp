#pragma once

namespace rcgp {

// Emulating for loops with index sequences
#define constexpr_for(name, N, ...)				\
	[&] <size_t ... name> (std::index_sequence <name...>) {	\
		__VA_ARGS__;					\
	} (std::make_index_sequence <N> ())

// Shorthand for casting
#define Tas static_cast

// Defining type traits
#define TYPE_TRAIT(name) 						\
	template <typename T> struct name : std::false_type {};		\
	template <typename T> constexpr bool name##_v = name <T> ::value
#define TYPE_TRAIT_INCLUDES(name, ...) struct name <__VA_ARGS__> : std::true_type {}

} // namespace rcgp
