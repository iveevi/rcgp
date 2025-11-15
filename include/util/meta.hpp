#pragma once

#include <tuple>

template <typename ... Args>
struct sequence {
	sequence(Args ...) {}

	template <typename T>
	using append_t = sequence <Args..., T>;

	using tuple = std::tuple <Args...>;
};
