#pragma once

#include <stack>
#include <string>
#include <unordered_map>

#include "instruction_nodes.hpp"

namespace rcgp {

struct Tracer {
	std::stack <SharedBlockReference> records;
	std::unordered_map <std::string, Reference> type_cache;

	Block &active();

	static thread_local Tracer singleton;
};

#define $tsb Tracer::singleton.active()

} // namespace rcgp
