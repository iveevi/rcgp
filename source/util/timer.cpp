#include <algorithm>
#include <list>
#include <memory>
#include <string>
#include <map>

#include <fmt/color.h>
#include <fmt/format.h>

#include "util/logging.hpp"
#include "util/timer.hpp"

static size_t display_width(const std::string &text)
{
	size_t width = 0;
	for (unsigned char ch : text) {
		if ((ch & 0xC0) != 0x80)
			++width;
	}
	return width;
}

struct ReportWidths {
	size_t label = 0;
	size_t ms = 0;
	size_t pct = 0;
};

ReportWidths merge_widths(const ReportWidths &a, const ReportWidths &b)
{
	return {
		std::max(a.label, b.label),
		std::max(a.ms, b.ms),
		std::max(a.pct, b.pct),
	};
}

ReportWidths measure_widths(
	const TimerToken::Payload &payload,
	const std::string &prefix,
	bool is_last,
	bool has_parent,
	double parent = -1
)
{
	const std::string line_prefix = has_parent
		? prefix + (is_last ? "└─ " : "├─ ")
		: std::string();
	const std::string label = fmt::format("{}:", payload.name);
	const std::string ms_str = fmt::format("{:.2f} ms", payload.milliseconds);
	size_t pct_len = 0;
	if (parent > 0) {
		const std::string pct_str = fmt::format("({:2.2f}%)", 100 * payload.milliseconds / parent);
		pct_len = 1 + pct_str.size();
	}

	ReportWidths widths {
		display_width(line_prefix) + label.size(),
		ms_str.size(),
		pct_len,
	};

	std::string child_prefix = prefix;
	if (has_parent)
		child_prefix += is_last ? "   " : "│  ";

	const size_t nested_count = payload.nested.size();
	const size_t note_count = payload.notes.size();
	const size_t total = note_count + nested_count;
	size_t idx = 0;

	if (note_count > 0) {
		for (auto &note : payload.notes) {
			const size_t note_label_width = display_width(child_prefix) + 3 + 1 + display_width(note);
			widths.label = std::max(widths.label, note_label_width);
		}
	}

	for (auto &nested_payload : payload.nested) {
		const bool last = (++idx == total);
		widths = merge_widths(widths, measure_widths(
			*nested_payload, child_prefix, last, true, payload.milliseconds
		));
	}

	return widths;
}

std::string report_impl(
	const TimerToken::Payload &payload,
	const std::string &prefix,
	bool is_last,
	bool has_parent,
	const ReportWidths &widths,
	double parent = -1
)
{
	const std::string line_prefix = has_parent
		? prefix + (is_last ? "└─ " : "├─ ")
		: std::string();
	const std::string label = fmt::format("{}:", payload.name);
	const std::string ms_str = fmt::format("{:.2f} ms", payload.milliseconds);

	std::string result;
	result += line_prefix + label;
	const size_t label_len = display_width(line_prefix) + label.size();
	const size_t ms_col = widths.label + 1;
	const size_t label_pad = ms_col > label_len ? ms_col - label_len : 1;
	result += std::string(label_pad, ' ');
	const size_t ms_pad = widths.ms > ms_str.size() ? widths.ms - ms_str.size() : 0;
	result += std::string(ms_pad, ' ');
	result += fmt::format(fmt::fg(fmt::color::gray), "{}", ms_str);

	const size_t current_col = ms_col + widths.ms;
	const size_t pct_col = current_col + 1;
	const size_t pct_pad = pct_col > current_col ? pct_col - current_col : 0;
	if (parent > 0) {
		const std::string pct_str = fmt::format("({:.2f}%)", 100 * payload.milliseconds / parent);
		result += std::string(pct_pad, ' ');
		result += fmt::format(
			fmt::emphasis::italic | fmt::fg(fmt::color::gray),
			"{}", pct_str
		);
	} else if (widths.pct > 0) {
		result += std::string(pct_pad + widths.pct, ' ');
	}
	result += "\n";

	std::string child_prefix = prefix;
	if (has_parent)
		child_prefix += is_last ? "   " : "│  ";

	const size_t note_count = payload.notes.size();
	const size_t nested_count = payload.nested.size();
	const size_t total = note_count + nested_count;
	size_t idx = 0;

	for (auto &note : payload.notes) {
		const bool last = (++idx == total);
		std::string label_note = fmt::format(
			fmt::emphasis::italic | fmt::fg(fmt::color::gray),
			"!{}", note
		);
		result += child_prefix + (last ? "└─ " : "├─ ") + label_note;
		result += "\n";
	}

	for (auto &nested_payload : payload.nested) {
		const bool last = (++idx == total);
		result += report_impl(*nested_payload, child_prefix, last, true, widths, payload.milliseconds);
	}

	return result;
}

thread_local TimerToken::clock TimerToken::clk;
thread_local std::stack <std::shared_ptr <TimerToken::Payload>> TimerToken::active;
thread_local std::map <std::string, TimerToken::PayloadCallback> payload_callbacks;

std::string TimerToken::Payload::report_string() const
{
	auto widths = measure_widths(*this, "", true, false);
	return report_impl(*this, "", true, false, widths);
}

TimerToken::TimerToken(const std::string &name)
{
	begin(name);
}

void TimerToken::begin(const std::string &name)
{
	payload = std::make_shared <Payload> ();
	payload->name = name;
	payload->begin = clk.now();

	if (not active.empty()) {
		auto &top = active.top();
		top->nested.emplace_back(payload);
	}

	active.push(payload);
}

void TimerToken::end()
{
	using std::chrono::duration_cast;
	using std::chrono::microseconds;

	auto us = duration_cast <microseconds> (
		clk.now() - payload->begin
	).count();

	payload->milliseconds = double(us) / 1000.0;

	assertion(active.top() == payload, "scoped timer invariant was broken");
	active.pop();

	if (active.empty()) {
		auto callbacks = payload_callbacks;
		for (auto &[_, callback] : callbacks)
			callback(*payload);
	}
}

void TimerToken::note(const std::string &note)
{
	assertion(not active.empty(), "cannot add notes when there is no active scope timer");
	auto &top = active.top();
	top->notes.emplace_back(note);
}

void TimerToken::entry(const std::string &name, double ms)
{
	assertion(not active.empty(), "cannot add entry when there is no active scope timer");

	auto entry_payload = std::make_shared <Payload> ();
	entry_payload->name = name;
	entry_payload->milliseconds = ms;

	auto &top = active.top();
	top->nested.emplace_back(entry_payload);
}

void TimerToken::add_callback(const std::string &name, PayloadCallback callback)
{
	payload_callbacks.emplace(name, std::move(callback));
}

void TimerToken::add_default_callback()
{
	add_callback("default",
		[](const Payload &payload) {
			info("scoped timer payload report:\n%s",
				payload.report_string().c_str());
		}
	);
}

void TimerToken::remove_callback(const std::string &name)
{
	payload_callbacks.erase(name);
}

void TimerToken::clear_callbacks()
{
	payload_callbacks.clear();
}
