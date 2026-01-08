#include "util/logging.hpp"

#include <cstdio>
#include <cstdarg>

namespace {

constexpr const char *COLOR_RESET = "\x1b[0m";
constexpr const char *COLOR_INFO = "\x1b[94m";
constexpr const char *COLOR_WARNING = "\x1b[93m";
constexpr const char *COLOR_ERROR = "\x1b[91m";
constexpr const char *COLOR_OK = "\x1b[92m";
constexpr const char *COLOR_ASSERT = "\x1b[31m";
constexpr const char *COLOR_FATAL = "\x1b[95m";
constexpr const char *STYLE_BOLD = "\x1b[1m";
constexpr const char *STYLE_FAINT = "\x1b[2m";

void logv(const char *level, const char *color, const char *fmt_str, va_list args)
{
	std::fputs(STYLE_BOLD, stderr);
	std::fputs("rcgp:", stderr);
	std::fputs(COLOR_RESET, stderr);
	std::fputs(" ", stderr);
	std::fputs(color, stderr);
	std::fputs(STYLE_BOLD, stderr);
	std::fputs(level, stderr);
	std::fputs(":", stderr);
	std::fputs(COLOR_RESET, stderr);
	std::fputs(" ", stderr);
	std::fputs(STYLE_FAINT, stderr);
	std::vfprintf(stderr, fmt_str, args);
	std::fputs(COLOR_RESET, stderr);
	std::fputc('\n', stderr);
}

} // namespace

void info(const char *fmt_str, ...)
{
	va_list args;
	va_start(args, fmt_str);
	logv("info", COLOR_INFO, fmt_str, args);
	va_end(args);
}

void warning(const char *fmt_str, ...)
{
	va_list args;
	va_start(args, fmt_str);
	logv("warning", COLOR_WARNING, fmt_str, args);
	va_end(args);
}

void error(const char *fmt_str, ...)
{
	va_list args;
	va_start(args, fmt_str);
	logv("error", COLOR_ERROR, fmt_str, args);
	va_end(args);
}

void ok(const char *fmt_str, ...)
{
	va_list args;
	va_start(args, fmt_str);
	logv("ok", COLOR_OK, fmt_str, args);
	va_end(args);
}

void assertion(bool condition, const char *fmt_str, ...)
{
	if (condition) return;
	va_list args;
	va_start(args, fmt_str);
	logv("assertion failed", COLOR_ASSERT, fmt_str, args);
	va_end(args);
	__builtin_trap();
}

[[noreturn]] void fatal(const char *fmt_str, ...)
{
	va_list args;
	va_start(args, fmt_str);
	logv("fatal error", COLOR_FATAL, fmt_str, args);
	va_end(args);
	__builtin_trap();
}
