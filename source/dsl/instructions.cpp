#include "dsl/instructions.hpp"

std::string swizzle_string(SwizzleCode code)
{
	static constexpr char letters[] = "xyzw";

	auto raw = static_cast <int> (code);
	if (raw < 0)
		return "?";

	int length = 1;
	int offset = 0;
	int count = 4;
	while (length <= 4 && raw >= offset + count) {
		offset += count;
		count *= 4;
		length++;
	}

	if (length > 4 || raw < offset)
		return "?";

	int index = raw - offset;
	int divisor = 1;
	for (int i = 1; i < length; i++)
		divisor *= 4;

	std::string result;
	result.reserve(length);
	for (int i = 0; i < length; i++) {
		int digit = index / divisor;
		index %= divisor;
		result.push_back(letters[digit]);
		divisor /= 4;
	}

	return result;
}

void Block::apply_group_allocation_map(const group_allocation_map &map)
{
	for (auto &[addr, group] : map) {
		auto &refs = context.global_resources[addr];
		for (auto &ref : refs)
			ref->as <GlobalResource> ().group = group;
	}
}

void Block::apply_push_constant_allocation_map(const push_constant_allocation_map &map)
{
	for (auto &[addr, layout] : map) {
		auto &refs = context.global_resources[addr];
		for (auto &ref : refs) {
			auto &grsrc = ref->as <GlobalResource> ();
			grsrc.push_constant_index = layout.index;
			grsrc.push_constant_offset = layout.offset;
		}
	}
}

void Block::Context::add_argument(Argument arg)
{
	if (arguments.size() > arg.argi) {
		// already registered
		__builtin_trap();
	} else {
		arguments.resize(arg.argi + 1);
		arguments[arg.argi] = arg;
	}
}

void Block::Context::add_thread_input(ThreadInput tin)
{
	if (thread_inputs.size() > tin.argi) {
		// already registered
		__builtin_trap();
	} else {
		thread_inputs.resize(tin.argi + 1);
		thread_inputs[tin.argi] = tin;
	}
}

void Block::Context::add_thread_output(ThreadOutput tout)
{
	if (thread_outputs.size() > tout.argi) {
		// already registered
		// TODO: this is fine, just make sure its the same or
		// its uninitialized...
		__builtin_trap();
	} else {
		thread_outputs.resize(tout.argi + 1);
		thread_outputs[tout.argi] = tout;
	}
}
