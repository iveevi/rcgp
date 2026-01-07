#pragma once

#include <cstdarg>

void info(const char *fmt_str, ...);
void warning(const char *fmt_str, ...);
void error(const char *fmt_str, ...);
void ok(const char *fmt_str, ...);

void assertion(bool condition, const char *fmt_str, ...);
[[noreturn]] void fatal(const char *fmt_str, ...);
