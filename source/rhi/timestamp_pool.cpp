#include <cstdio>
#include <cstdlib>

#include "rhi/timestamp_pool.hpp"

namespace rcgp {

std::optional <double> TimestampQueryResult::delta(size_t a, size_t b) const
{
	if (!ready()) {
		std::fputs("cannot query deltas for unready time stamps\n", stderr);
		std::abort();
	}

	auto K = period / 1'000'000.0f;
	if (flags & VK_QUERY_RESULT_WITH_AVAILABILITY_BIT) {
		if (stamps[2 * a + 1] != 0 && stamps[2 * b - 1] != 0)
			return double(stamps[2 * b] - stamps[2 * a]) * K;
		return std::nullopt;
	}

	return double(stamps[b] - stamps[a]) * K;
}

} // namespace rcgp
