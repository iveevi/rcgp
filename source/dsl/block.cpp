#include "dsl/block.hpp"
#include "dsl/instructions.hpp"

namespace rcgp {

void Block::apply_group_allocation_map(const group_allocation_map &map)
{
	for (auto &[addr, group] : map) {
		if (global_resources.contains(addr)) {
			auto &ref = global_resources.at(addr);
			ref->as <GlobalResource> ().group = group;
		}
	}
}

void Block::apply_push_constant_allocation_map(const push_constant_allocation_map &map)
{
	for (auto &[addr, offset] : map) {
		if (global_resources.contains(addr)) {
			auto &ref = global_resources.at(addr);
			ref->as <GlobalResource> ().offset = offset;
		}
	}
}

void Block::add_argument(const Argument &arg)
{
	if (arguments.size() > arg.argi) {
		// already registered
		__builtin_trap();
	} else {
		arguments.resize(arg.argi + 1);
		arguments[arg.argi] = arg;
	}
}

void Block::add_return(const Return &ret)
{
	if (returns.size() > ret.argi) {
		// already registered
		__builtin_trap();
	} else {
		returns.resize(ret.argi + 1);
		returns[ret.argi] = ret;
	}
}

void Block::add_stage_input(const StageInput &sin)
{
	if (stage_inputs.size() > sin.argi) {
		// already registered
		__builtin_trap();
	} else {
		stage_inputs.resize(sin.argi + 1);
		stage_inputs[sin.argi] = sin;
	}
}

void Block::add_stage_output(const StageOutput &sout)
{
	if (stage_outputs.size() > sout.argi) {
		// already registered
		// TODO: this is fine, just make sure its the same or
		// its uninitialized...
		__builtin_trap();
	} else {
		stage_outputs.resize(sout.argi + 1);
		stage_outputs[sout.argi] = sout;
	}
}

void Block::add_global_resource(void *addr, const Reference &resource)
{
	global_resources.emplace(addr, resource);
}

void Block::set_workgroup_size(uint32_t x, uint32_t y, uint32_t z)
{
	auto size = std::array <uint32_t, 3> { x, y, z };
	if (workgroup_size.has_value() && workgroup_size.value() != size) {
		__builtin_trap();
	}
	workgroup_size = size;
}

Reference Block::add(const Instruction &instr)
{
	auto ref = std::make_shared <Instruction> (instr);
	push_back(ref);
	return ref;
}

} // namespace rcgp
