#pragma once

#include <stack>

#include <fmt/printf.h>

#include "instructions.hpp"

// NOTE: this tracer is not limited to just shaders...
// it also includes game engine stuff/code
struct Tracer {
	std::stack <std::reference_wrapper <Block>> records;

	Block &active() {
		if (records.empty()) {
			fmt::println(stderr, "no active record");
			__builtin_trap();
		}

		return records.top();
	}

	// Singleton
	static thread_local Tracer singleton;
};

inline thread_local Tracer Tracer::singleton;

#define $tsb Tracer::singleton.active()
