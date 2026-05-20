#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <string>
#include <string_view>
#include <sys/wait.h>

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

std::string read_file_contents(const std::filesystem::path &path)
{
	std::ifstream fin(path);
        if (!fin.good()) {
                std::println("failed to open file: {}", path.c_str());
		std::abort();
        }

        std::stringstream s;
        s << fin.rdbuf();
        return s.str();
}

// unifdef the fixture against the host compiler's identity macros.
static std::string read_expected(const std::filesystem::path &path)
{
#if defined(__clang__)
	auto cmd = fmt::format("unifdef -t -D__clang__ -U__GNUC__ {}", path.c_str());
#elif defined(__GNUC__)
	auto cmd = fmt::format("unifdef -t -U__clang__ -D__GNUC__ -D__GNUC__={} {}", __GNUC__, path.c_str());
#else
	auto cmd = fmt::format("unifdef -t {}", path.c_str());
#endif

	auto *pipe = popen(cmd.c_str(), "r");
	if (!pipe) {
		std::println("failed to invoke unifdef on {}", path.c_str());
		std::abort();
	}

	std::string out;
	std::array <char, 4096> buf;
	while (auto n = std::fread(buf.data(), 1, buf.size(), pipe))
		out.append(buf.data(), n);

	auto status = pclose(pipe);
	auto code = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
	if (code != 0 && code != 1) {
		std::println("unifdef failed ({}) on {}", code, path.c_str());
		std::abort();
	}

	return out;
}

static void match_against_file(const std::filesystem::path &path, const std::string &act)
{
	auto full_path = "tests/dsl" / path;
	auto expected = read_expected(full_path);
	if (expected.empty()) {
		if (tests.update_fixtures) {
			std::ofstream fout(full_path);
			fout << act;
			return;
		}
		mark_fail;
		return;
	}

	if (act != expected) {
		if (!tests.actual_suffix.empty()) {
			auto sidecar = full_path;
			sidecar += "." + tests.actual_suffix;
			std::ofstream fout(sidecar);
			fout << act;
		}

		if (tests.update_fixtures) {
			std::ofstream fout(full_path);
			fout << act;
			return;
		}

		show_diff(expected, act);
		if (tests.show_ground_truth) {
			auto style = fmt::fg(fmt::color::gray)
				| fmt::emphasis::italic;
			fmt::print(style, "ground truth:\n{}\n", act);
		}

		mark_fail;
	}
}

void assert_glsl_eq(
	const SharedBlockReference &block,
	const std::filesystem::path &path
)
{
	match_against_file(path, generate_glsl(block));
}

void assert_assembly_eq(
	const SharedBlockReference &block,
	const std::filesystem::path &path,
	bool verbose
)
{
	match_against_file(path, generate_assembly(block, false, verbose));
}
