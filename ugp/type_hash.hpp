#pragma once

#include <cstdlib>
#include <string_view>

namespace detail {

consteval size_t hash_string(std::string_view str)
{
	size_t hash = 14695981039346656037ULL;
	for (char c : str) {
		hash ^= static_cast <size_t> (c);
		hash *= 1099511628211ULL;
	}

	return hash;
}

template <typename T>
consteval size_t compute_type_hash()
{
#if defined(__clang__)
	constexpr std::string_view name = __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
	constexpr std::string_view name = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
	constexpr std::string_view name = __FUNCSIG__;
#else
	constexpr std::string_view name = "unknown";
#endif
	return hash_string(name);
}

}

template <typename T>
inline constexpr size_t type_hash_v = detail::compute_type_hash <T> ();

