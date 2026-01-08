#pragma once

#include <stack>

#include "instructions.hpp"
#include "../util/logging.hpp"

struct Tracer {
	std::stack <SharedBlockReference> records;

	Block &active() {
		if (records.empty())
			fatal("no active record");
		return *records.top();
	}

	static thread_local Tracer singleton;
};

inline thread_local Tracer Tracer::singleton;

#define $tsb Tracer::singleton.active()
