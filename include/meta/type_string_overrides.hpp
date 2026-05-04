#pragma once

#include <cstddef>
#include <string_view>
#include <type_traits>

#include "../dsl/aliases.hpp"
#include "static_string.hpp"

namespace rcgp {

// $ss_type(T) overrides for DSL types. Variadic so the type's own commas
// don't break macro arg parsing.
#define RCGP_TYPE_NAME(NAME, ...)				\
	template <>						\
	struct type_string <__VA_ARGS__> {			\
		template <size_t I>				\
		static consteval auto eval() { return NAME; }	\
	}

RCGP_TYPE_NAME("boolean"_ss,    scalar <bool>);
RCGP_TYPE_NAME("i32"_ss,        scalar <int32_t>);
RCGP_TYPE_NAME("u32"_ss,        scalar <uint32_t>);
RCGP_TYPE_NAME("f32"_ss,        scalar <float>);

RCGP_TYPE_NAME("f32[2]"_ss,     vector <float, 2>);
RCGP_TYPE_NAME("f32[3]"_ss,     vector <float, 3>);
RCGP_TYPE_NAME("f32[4]"_ss,     vector <float, 4>);
RCGP_TYPE_NAME("i32[2]"_ss,     vector <int32_t, 2>);
RCGP_TYPE_NAME("i32[3]"_ss,     vector <int32_t, 3>);
RCGP_TYPE_NAME("i32[4]"_ss,     vector <int32_t, 4>);
RCGP_TYPE_NAME("u32[2]"_ss,     vector <uint32_t, 2>);
RCGP_TYPE_NAME("u32[3]"_ss,     vector <uint32_t, 3>);
RCGP_TYPE_NAME("u32[4]"_ss,     vector <uint32_t, 4>);

RCGP_TYPE_NAME("f32[3][3]"_ss,  matrix <float, 3, 3>);
RCGP_TYPE_NAME("f32[4][4]"_ss,  matrix <float, 4, 4>);
RCGP_TYPE_NAME("f32[4][3]"_ss,  matrix <float, 4, 3>);

#undef RCGP_TYPE_NAME

// Render an `auto &ref` non-type template argument as just its variable name.
template <auto &ref>
consteval std::string_view ref_name_view()
{
#if defined(__clang__)
	std::string_view f = __PRETTY_FUNCTION__;
	auto key = std::string_view { "[ref = " };
	auto start = f.find(key);
	start = (start == std::string_view::npos) ? 0 : start + key.size();
	auto end = f.rfind(']');
	auto qualified = f.substr(start, end - start);
	auto last_colon = qualified.rfind("::");
	return (last_colon == std::string_view::npos)
		? qualified
		: qualified.substr(last_colon + 2);
#elif defined(__GNUC__)
	std::string_view f = __PRETTY_FUNCTION__;
	auto key = std::string_view { "with auto& ref = " };
	auto start = f.find(key);
	start = (start == std::string_view::npos) ? 0 : start + key.size();
	auto end = f.find(';', start);
	if (end == std::string_view::npos)
		end = f.rfind(']');
	auto qualified = f.substr(start, end - start);
	auto last_colon = qualified.rfind("::");
	return (last_colon == std::string_view::npos)
		? qualified
		: qualified.substr(last_colon + 2);
#else
#error Unsupported compiler
#endif
}

template <auto &ref>
consteval auto ref_name_string()
{
	constexpr auto v = ref_name_view <ref> ();
	static_string <v.size()> result;
	for (size_t i = 0; i < v.size(); i++)
		result.elements[i] = v[i];
	return result;
}

#define $ss_ref_name(REF)	::rcgp::ref_name_string <REF> ()

// Render a `void *Addr` non-type template argument as just its variable name.
template <void *Addr>
consteval std::string_view addr_name_view()
{
#if defined(__clang__)
	std::string_view f = __PRETTY_FUNCTION__;
	auto key = std::string_view { "[Addr = " };
	auto start = f.find(key);
	start = (start == std::string_view::npos) ? 0 : start + key.size();
	auto end = f.rfind(']');
	auto qualified = f.substr(start, end - start);
	if (!qualified.empty() && qualified[0] == '&')
		qualified = qualified.substr(1);
	auto last_colon = qualified.rfind("::");
	return (last_colon == std::string_view::npos)
		? qualified
		: qualified.substr(last_colon + 2);
#elif defined(__GNUC__)
	std::string_view f = __PRETTY_FUNCTION__;
	auto key = std::string_view { "with void* Addr = " };
	auto start = f.find(key);
	start = (start == std::string_view::npos) ? 0 : start + key.size();
	auto end = f.find(';', start);
	if (end == std::string_view::npos)
		end = f.rfind(']');
	auto qualified = f.substr(start, end - start);
	if (!qualified.empty() && qualified[0] == '&')
		qualified = qualified.substr(1);
	auto last_colon = qualified.rfind("::");
	return (last_colon == std::string_view::npos)
		? qualified
		: qualified.substr(last_colon + 2);
#else
#error Unsupported compiler
#endif
}

template <void *Addr>
consteval auto addr_name_string()
{
	constexpr auto v = addr_name_view <Addr> ();
	static_string <v.size()> result;
	for (size_t i = 0; i < v.size(); i++)
		result.elements[i] = v[i];
	return result;
}

#define $ss_addr_name(ADDR)	::rcgp::addr_name_string <ADDR> ()

} // namespace rcgp
