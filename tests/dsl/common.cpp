#include <algorithm>
#include <cstddef>
#include <fstream>
#include <string>
#include <string_view>

#include <fmt/color.h>
#include <fmt/printf.h>

#include "common.hpp"

enum class DiffKind {
	Equal,
	Insert,
	Delete,
};

struct DiffOp {
	DiffKind kind;
	size_t expected_index;
	size_t actual_index;
};

std::vector <std::string_view> split_lines(std::string_view text)
{
	std::vector <std::string_view> lines;
	size_t start = 0;
	while (start <= text.size()) {
		size_t end = text.find('\n', start);
		if (end == std::string_view::npos)
			end = text.size();
		lines.emplace_back(text.substr(start, end - start));
		if (end == text.size())
			break;
		start = end + 1;
	}
	if (!lines.empty() && lines.back().empty() && text.ends_with('\n'))
		lines.pop_back();
	return lines;
}

std::vector <DiffOp> diff_lines(
	const std::vector <std::string_view> &expected,
	const std::vector <std::string_view> &actual
)
{
	const size_t n = expected.size();
	const size_t m = actual.size();
	std::vector <size_t> dp((n + 1) * (m + 1), 0);
	auto idx = [&](size_t i, size_t j) {
		return i * (m + 1) + j;
	};
	for (size_t i = 1; i <= n; i++) {
		for (size_t j = 1; j <= m; j++) {
			if (expected[i - 1] == actual[j - 1]) {
				dp[idx(i, j)] = dp[idx(i - 1, j - 1)] + 1;
			} else {
				dp[idx(i, j)] = std::max(dp[idx(i - 1, j)], dp[idx(i, j - 1)]);
			}
		}
	}

	std::vector <DiffOp> ops;
	size_t i = n;
	size_t j = m;
	while (i > 0 || j > 0) {
		if (i > 0 && j > 0 && expected[i - 1] == actual[j - 1]) {
			ops.push_back({DiffKind::Equal, i - 1, j - 1});
			i--;
			j--;
		} else if (j > 0 && (i == 0 || dp[idx(i, j - 1)] >= dp[idx(i - 1, j)])) {
			ops.push_back({DiffKind::Insert, i, j - 1});
			j--;
		} else {
			ops.push_back({DiffKind::Delete, i - 1, j});
			i--;
		}
	}

	std::reverse(ops.begin(), ops.end());
	return ops;
}

void show_diff(const std::string &expected_str, const std::string &actual_str)
{
	auto expected_lines = split_lines(expected_str);
	auto actual_lines = split_lines(actual_str);
	auto ops = diff_lines(expected_lines, actual_lines);

	size_t max_len = 0;
	for (const auto &line : expected_lines)
		max_len = std::max(max_len, line.size());
	for (const auto &line : actual_lines)
		max_len = std::max(max_len, line.size());

	const auto add_style = fmt::fg(fmt::color::white) | fmt::bg(fmt::color::dark_green);
	const auto del_style = fmt::fg(fmt::color::white) | fmt::bg(fmt::color::dark_red);
	const auto same_style = fmt::fg(fmt::color::gray);

	fmt::println("(-) expected (+) actual");

	for (const auto &op : ops) {
		std::string_view line;
		switch (op.kind) {
		case DiffKind::Equal:
			line = expected_lines[op.expected_index];
			break;
		case DiffKind::Insert:
			line = actual_lines[op.actual_index];
			break;
		case DiffKind::Delete:
			line = expected_lines[op.expected_index];
			break;
		}

		auto padded = fmt::format("{:<{}}", line, max_len);
		switch (op.kind) {
		case DiffKind::Equal:
			fmt::print(same_style, "  {}\n", padded);
			break;
		case DiffKind::Insert:
			fmt::print(add_style, "+ {}", padded);
			fmt::print("\n");
			break;
		case DiffKind::Delete:
			fmt::print(del_style, "- {}", padded);
			fmt::print("\n");
			break;
		}
	}
}

std::string clean(const std::string &input)
{
	auto lines = split_lines(input);

	std::string result;
	for (size_t i = 1; i < lines.size(); i++) {
		size_t off = 0;
		if (lines[i].size() && lines[i][0] == '\t')
			off++;

		result += lines[i].substr(off);
		if (i + 1 <  lines.size())
			result += '\n';
	}

	return result;
}

void assert_assembly_match(const SharedBlockReference &block, const std::string &str)
{
	auto expected = clean(str);
	auto act = generate_assembly(block);
	if (act != expected) {
		show_diff(expected, act);
		if (tests.show_ground_truth) {
			auto style = fmt::fg(fmt::color::gray)
				| fmt::emphasis::italic;
			fmt::print(style, "ground truth:\n{}\n", act);
		}

		mark_fail;
	}
}

void assert_glsl_match(const SharedBlockReference &block, const std::string &str)
{
	auto expected = clean(str);
	auto act = generate_glsl(block);
	if (act != expected) {
		show_diff(expected, act);
		if (tests.show_ground_truth) {
			auto style = fmt::fg(fmt::color::gray)
				| fmt::emphasis::italic;
			fmt::print(style, "ground truth:\n{}\n", act);
		}

		mark_fail;
	}
}

std::string read_file_contents(const std::filesystem::path &path)
{
	std::ifstream fin(path);
        if (!fin.good()) {
                fmt::println("failed to open file: {}", path.c_str());
		std::abort();
        }

        std::stringstream s;
        s << fin.rdbuf();
        return s.str();
}

void assert_glsl_match_file(const SharedBlockReference &block, const std::filesystem::path &path)
{
	auto expected = read_file_contents("tests/dsl" / path);
	if (expected.empty()) {
		mark_fail;
		return;
	}

	auto act = generate_glsl(block);
	if (act != expected) {
		show_diff(expected, act);
		if (tests.show_ground_truth) {
			auto style = fmt::fg(fmt::color::gray)
				| fmt::emphasis::italic;
			fmt::print(style, "ground truth:\n{}\n", act);
		}

		mark_fail;
	}
}
