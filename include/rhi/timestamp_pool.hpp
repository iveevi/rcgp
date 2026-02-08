#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "vk.hpp"

namespace rcgp {

struct TimestampQueryResult {
	std::vector <uint64_t> stamps;
	VkQueryResultFlags flags;
	double period;

	std::optional <double> delta(size_t a, size_t b) const;

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
