#include <fmt/printf.h>
#include <fmt/color.h>

#include "common.hpp"

int main(int argc, char *argv[])
{
	size_t argi = 1;
	while (argi < argc) {
		if (argv[argi] == std::string("--gt"))
			g_suite.show_ground_truth = true;
		argi++;
	}

	auto pass_style = fmt::emphasis::bold | fmt::fg(fmt::color::light_green);
	auto fail_style = fmt::emphasis::bold | fmt::fg(fmt::color::orange_red);

	std::vector <std::string> failing;
	for (auto &test : g_suite.tests) {
		g_suite.pass = true;

		test.impl();

		if (g_suite.pass) {
			fmt::print(pass_style, "passed: ");
		} else {
			fmt::print(fail_style, "failed: ");
			failing.push_back(test.name);
		}

		fmt::println("{}", test.name);
	}

	if (failing.size()) {
		fmt::print(fail_style, "summary: ");
		fmt::print("{}/{} tests failed:\n", failing.size(), g_suite.tests.size());
		for (auto &name : failing)
			fmt::print(fmt::fg(fmt::color::gray), "  {}\n", name);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
