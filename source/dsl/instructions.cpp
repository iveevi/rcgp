#include "dsl/instructions.hpp"

namespace rcgp {

template <typename T>
Reference Block::add(const T &sub, const Debug aux)
{
	auto result = std::make_shared <Instruction> (sub, aux);
	emplace_back(result);
	return result;
}

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

// TODO: use a script to generate instantiations
template Reference Block::add <Argument> (const Argument &sub, Debug aux);
template Reference Block::add <Block> (const Block &sub, Debug aux);
template Reference Block::add <BuiltinIntrinsic> (const BuiltinIntrinsic &sub, Debug aux);
template Reference Block::add <Constant> (const Constant &sub, Debug aux);
template Reference Block::add <Construct> (const Construct &sub, Debug aux);
template Reference Block::add <Invocation> (const Invocation &sub, Debug aux);
template Reference Block::add <ArrayAccess> (const ArrayAccess &sub, Debug aux);
template Reference Block::add <FieldAccess> (const FieldAccess &sub, Debug aux);
template Reference Block::add <GlobalIntrinsic> (const GlobalIntrinsic &sub, Debug aux);
template Reference Block::add <GlobalResource> (const GlobalResource &sub, Debug aux);
template Reference Block::add <Branch> (const Branch &sub, Debug aux);
template Reference Block::add <Loop> (const Loop &sub, Debug aux);
template Reference Block::add <Local> (const Local &sub, Debug aux);
template Reference Block::add <Operation> (const Operation &sub, Debug aux);
template Reference Block::add <Store> (const Store &sub, Debug aux);
template Reference Block::add <Swizzle> (const Swizzle &sub, Debug aux);
template Reference Block::add <StageInput> (const StageInput &sub, Debug aux);
template Reference Block::add <StageOutput> (const StageOutput &sub, Debug aux);
template Reference Block::add <Type> (const Type &sub, Debug aux);

} // namespace rcgp
