#pragma once

#include <iostream>
#include <print>

#define assertion(c) 								\
	if (not (c)) {								\
		std::println(std::cerr, "rcgp: assertion failed: {}", #c);	\
		std::flush(std::cerr);						\
		__builtin_trap();						\
	}

#define fatal(...) {								\
		std::println(std::cerr, "rcgp: " __VA_ARGS__);			\
		std::flush(std::cerr);						\
		__builtin_trap();						\
	}
