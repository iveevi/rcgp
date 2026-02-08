#pragma once

#include <cstdlib>
#include <iostream>
#include <print>

namespace rcgp {

struct TimestampQueryResult {
	std::vector <uint64_t> stamps;
	VkQueryResultFlags flags;
	double period;

	std::optional <double> delta(size_t a, size_t b) const {
		if (!ready()) {
			std::println(std::cerr, "cannot query deltas for unready time stamps");
			std::abort();
		}

		auto K = period / 1'000'000.0f;
		if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
			if (stamps[2 * a + 1] != 0 && stamps[2 * b - 1] != 0)
				return double(stamps[2 * b] - stamps[2 * a]) * K;
			else
				return std::nullopt;
		} else {
			return double(stamps[b] - stamps[a]) * K;
		}
	}

	bool ready() const {
		return not stamps.empty();
	}
};

struct TimestampQueryPool {
	VkQueryPool handle = VK_NULL_HANDLE;
	VkQueryResultFlags flags = 0;
	double period;
	size_t count;
};

} // namespace rcgp
