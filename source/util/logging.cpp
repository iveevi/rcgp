#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

#include <fmt/color.h>
#include <fmt/format.h>

#include "util/logging.hpp"

std::string format_with_va_list(const char *fmt_str, va_list args)
{
	va_list copy;
	va_copy(copy, args);
	int size = std::vsnprintf(nullptr, 0, fmt_str, copy);
	va_end(copy);
	if (size <= 0)
		return "?";

	std::vector <char> buffer(size + 1);
	std::vsnprintf(buffer.data(), buffer.size(), fmt_str, args);
	return std::string(buffer.data(), size);
}

void logv(const char *level, fmt::color color, const char *fmt_str, va_list args)
{
	auto message = format_with_va_list(fmt_str, args);
	fmt::print(stderr, fmt::emphasis::bold, "rcgp:");
	fmt::print(stderr, " ");
	fmt::print(stderr, fmt::emphasis::bold | fmt::fg(color), "{}:", level);
	fmt::print(stderr, " ");
	fmt::print(stderr, fmt::emphasis::faint, "{}", message);
	fmt::print(stderr, "\n");
}

void info(const char *fmt_str, ...)
{
	va_list args;
	va_start(args, fmt_str);
	logv("info", fmt::color::light_blue, fmt_str, args);
	va_end(args);
}

void warning(const char *fmt_str, ...)
{
	va_list args;
	va_start(args, fmt_str);
	logv("warning", fmt::color::golden_rod, fmt_str, args);
	va_end(args);
}

void error(const char *fmt_str, ...)
{
	va_list args;
	va_start(args, fmt_str);
	logv("error", fmt::color::light_coral, fmt_str, args);
	va_end(args);
}

void ok(const char *fmt_str, ...)
{
	va_list args;
	va_start(args, fmt_str);
	logv("ok", fmt::color::green, fmt_str, args);
	va_end(args);
}

void assertion_impl(
	bool condition,
	const std::source_location &loc,
	const char *fmt_str,
	...
)
{
	if (condition) return;
	va_list args;
	va_start(args, fmt_str);
	logv("assertion failed", fmt::color::violet, fmt_str, args);
	va_end(args);
	fmt::print(stderr, fmt::emphasis::italic | fmt::fg(fmt::color::gray),
		"    at {}:{}\n", loc.file_name(), loc.line());
	__builtin_trap();
}

[[noreturn]] void fatal_impl(
	const std::source_location &loc,
	const char *fmt_str,
	...
)
{
	va_list args;
	va_start(args, fmt_str);
	logv("fatal error", fmt::color::purple, fmt_str, args);
	va_end(args);
	fmt::print(stderr, fmt::emphasis::italic | fmt::fg(fmt::color::gray),
		"    at {}:{}\n", loc.file_name(), loc.line());
	__builtin_trap();
}
