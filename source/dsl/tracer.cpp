#include <cstdio>
#include <cstdlib>

#include "dsl/tracer.hpp"

namespace rcgp {

Block &Tracer::active()
{
	if (records.empty()) {
		std::fputs("no active record\n", stderr);
		std::abort();
	}

	return *records.top();
}

thread_local Tracer Tracer::singleton;

} // namespace rcgp
