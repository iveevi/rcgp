#pragma once

#include <array>

template <size_t N>
constexpr int64_t first_on(const std::array <bool, N> &x)
{
	for (size_t i = 0; i < N; i++) {
		if (x[i])
			return i;
	}

	return -1;
}

template <typename T, size_t ... Ns>
constexpr auto concat(const std::array <T, Ns> &... arrays)
{
	std::array <T, (Ns + ...)> result {};
	size_t idx = 0;
	auto append = [&](const auto &arr) {
		for (const auto &el : arr)
			result[idx++] = el;
	};
	(append(arrays), ...);
	return result;
}
