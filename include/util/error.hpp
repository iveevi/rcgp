#pragma once

namespace rcgp {

[[noreturn]] void assertion_failed(const char *condition, const char *file, int line);
[[noreturn]] void fatalf(const char *format, ...);

} // namespace rcgp

#define assertion(c) do {							\
	if (not (c)) {							\
		::rcgp::assertion_failed(#c, __FILE__, __LINE__);	\
	}								\
} while (0)

#define fatal(...) do {					\
	::rcgp::fatalf(__VA_ARGS__);			\
} while (0)
