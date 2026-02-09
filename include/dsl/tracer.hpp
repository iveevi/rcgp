#pragma once

#include <stack>
#include <unordered_map>

#include "instruction_nodes.hpp"

namespace rcgp {

struct Tracer {
	std::stack <SharedBlockReference> records;
	std::unordered_map <std::string, Reference> type_cache;

	Block &active() {
		return *records.top();
	}

	static thread_local Tracer singleton;
};

inline thread_local Tracer Tracer::singleton;

#define $tsb Tracer::singleton.active()

} // namespace rcgp
