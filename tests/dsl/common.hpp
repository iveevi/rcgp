#pragma once

#include <functional>
#include <string>
#include <vector>

#include <rcgp.hpp>

using namespace rcgp;

using test_impl = std::function <void ()>;

struct test {
	std::string name;
	test_impl impl;
};

struct suite {
	bool pass;
	bool show_ground_truth;
	std::vector <test> tests;
} inline g_suite;

#define mark_fail g_suite.pass = false

inline auto operator*(const std::string &name, const test_impl &impl)
{
	return test(name, impl);
}

inline auto operator<<(suite &s, const test &item)
{
	s.tests.push_back(item);
	return item;
}

#define add_test(name) auto name = g_suite << #name * []

void assert_assembly_match(const SharedBlockReference &block, const std::string &str);
