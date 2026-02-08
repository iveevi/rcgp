#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "util/error.hpp"

namespace rcgp {

[[noreturn]] void assertion_failed(const char *condition, const char *file, int line)
{
	std::fprintf(stderr, "rcgp: assertion failed: %s (%s:%d)\n", condition, file, line);
	std::fflush(stderr);
	__builtin_trap();
}

[[noreturn]] void fatalf(const char *format, ...)
{
	std::fputs("rcgp: ", stderr);

	va_list args;
	va_start(args, format);
	std::vfprintf(stderr, format, args);
	va_end(args);

	std::fputc('\n', stderr);
	std::fflush(stderr);
	__builtin_trap();
}

} // namespace rcgp
