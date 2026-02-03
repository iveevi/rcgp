#include <fmt/printf.h>
#include <fmt/color.h>

#include "common.hpp"

int main(int argc, char *argv[])
{
	std::string suite;
	std::string filter;

	size_t argi = 1;
	while (argi < argc) {
		std::string arg = argv[argi];
		if (arg == "--gt")
			tests.show_ground_truth = true;
		else if (arg.starts_with("--suite="))
			suite = arg.substr(8);
		else if (arg.starts_with("--filter="))
			filter = arg.substr(9);

		argi++;
	}

	auto pass_style = fmt::emphasis::bold | fmt::fg(fmt::color::light_green);
	auto fail_style = fmt::emphasis::bold | fmt::fg(fmt::color::orange_red);

	size_t passed = 0;
	size_t skipped = 0;
	size_t ran = 0;

	std::vector <test> failing;
	for (auto &test : tests.tests) {
		if (suite.size() and test.suite != suite) {
			skipped++;
			continue;
		}

		if (filter.size() and not test.name.contains(filter)) {
			skipped++;
			continue;
		}

		tests.pass = true;
		test.impl();
		ran++;

		if (tests.pass) {
			fmt::print(pass_style, "passed: ");
			passed++;
		} else {
			fmt::print(fail_style, "failed: ");
			failing.push_back(test);
		}

		fmt::println("{}::{}", test.suite, test.name);
	}

	if (failing.size()) {
		fmt::print(fail_style, "summary: ");
		fmt::print("{}/{} tests failed:\n", failing.size(), ran);
		for (auto &test : failing)
			fmt::print(fmt::fg(fmt::color::gray), "  {}::{}\n", test.suite, test.name);
		return EXIT_FAILURE;
	} else {
		fmt::print(pass_style, "summary: ");
		if (skipped)
			fmt::print("passed {} tests, skipped {} tests\n", passed, skipped);
		else
			fmt::print("passed {} test\n", passed);
	}

	return EXIT_SUCCESS;
}
