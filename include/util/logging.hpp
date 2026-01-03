#pragma once

#include <fmt/printf.h>
#include <fmt/color.h>
#include <source_location>

template <typename ... Ts>
void info(fmt::format_string <Ts...> fmt_str, Ts &&...args)
{
	auto header = fmt::format(fmt::emphasis::bold, "rcgp:");
	auto alert = fmt::format(fmt::emphasis::bold | fmt::fg(fmt::color::blue), "info:");
	auto message = fmt::format(fmt_str, std::forward <Ts> (args)...);
	fmt::println(stderr, "{} {} {}", header, alert, message);
}

template <typename ... Ts>
void warning(fmt::format_string <Ts...> fmt_str, Ts &&...args)
{
	auto header = fmt::format(fmt::emphasis::bold, "rcgp:");
	auto alert = fmt::format(fmt::emphasis::bold | fmt::fg(fmt::color::yellow), "warning:");
	auto message = fmt::format(fmt_str, std::forward <Ts> (args)...);
	fmt::println(stderr, "{} {} {}", header, alert, message);
}

template <typename ... Ts>
void error(fmt::format_string <Ts...> fmt_str, Ts &&...args)
{
	auto header = fmt::format(fmt::emphasis::bold, "rcgp:");
	auto alert = fmt::format(fmt::emphasis::bold | fmt::fg(fmt::color::red), "error:");
	auto message = fmt::format(fmt_str, std::forward <Ts> (args)...);
	fmt::println(stderr, "{} {} {}", header, alert, message);
}

template <typename ... Ts>
void ok(fmt::format_string <Ts...> fmt_str, Ts &&...args)
{
	auto header = fmt::format(fmt::emphasis::bold, "rcgp:");
	auto alert = fmt::format(fmt::emphasis::bold | fmt::fg(fmt::color::green), "ok:");
	auto message = fmt::format(fmt_str, std::forward <Ts> (args)...);
	fmt::println(stderr, "{} {} {}", header, alert, message);
}

template <typename ... Ts>
void assertion(bool condition, fmt::format_string <Ts...> fmt_str, Ts &&...args)
{
	// TODO: source location...
	if (condition) return;
	auto header = fmt::format(fmt::emphasis::bold, "rcgp:");
	auto alert = fmt::format(fmt::emphasis::bold | fmt::fg(fmt::color::maroon), "assertion failed:");
	auto message = fmt::format(fmt_str, std::forward <Ts> (args)...);
	fmt::println(stderr, "{} {} {}", header, alert, message);
	__builtin_trap();
}

template <typename ... Ts>
[[noreturn]] void fatal(fmt::format_string <Ts...> fmt_str, Ts &&...args)
{
	// TODO: source location...
	auto header = fmt::format(fmt::emphasis::bold, "rcgp:");
	auto alert = fmt::format(fmt::emphasis::bold | fmt::fg(fmt::color::medium_purple), "fatal error:");
	auto message = fmt::format(fmt_str, std::forward <Ts> (args)...);
	fmt::println(stderr, "{} {} {}", header, alert, message);
	__builtin_trap();
}
