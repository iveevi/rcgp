#pragma once

#include <functional>
#include <string>
#include <vector>
#include <filesystem>

#include <rcgp.hpp>

using namespace rcgp;

using test_impl = std::function <void ()>;

struct test {
	std::string suite;
	std::string name;
	test_impl impl;
};

struct test_collection {
	bool pass;
	bool show_ground_truth;
	std::vector <test> tests;
} inline tests;

#define mark_fail tests.pass = false

inline auto operator*(const test &t, const test_impl &impl)
{
	return test(t.suite, t.name, impl);
}

inline auto operator<<(test_collection &s, const test &item)
{
	s.tests.push_back(item);
	return item;
}

#define add_test(name) static auto name = tests << test(SUITE, #name, nullptr) * []

inline auto operator*(std::nullptr_t, std::function <void ()> &&fn)
{
	Tracer::singleton.type_cache.clear();
	auto sbr = std::make_shared <Block> ();
	{ jems::scope scope(sbr); fn(); }
	sbr->name = "recorded";
	return sbr;
}

#define record nullptr * [&]

void assert_assembly_match(const SharedBlockReference &block, const std::string &str);
void assert_glsl_match(const SharedBlockReference &block, const std::string &str);

void assert_glsl_match_file(const SharedBlockReference &block, const std::filesystem::path &path);
