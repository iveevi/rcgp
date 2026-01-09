#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <vector>

#include "instruction_nodes.hpp"
#include "instruction_resources.hpp"

using group_allocation_map = std::map <void *, size_t>;

struct PushConstantAllocation {
	uint32_t index;
	uint32_t offset;
};

using push_constant_allocation_map = std::map <void *, PushConstantAllocation>;

struct Block : std::vector <Reference> {
	struct Context {
		ShaderStage model = ShaderStage::eSubroutine;

		std::vector <Argument> arguments;
		std::vector <ThreadInput> thread_inputs;
		std::vector <ThreadOutput> thread_outputs;
		std::map <void *, std::set <Reference>> global_resources;
		
		void add_argument(Argument arg);
		void add_thread_input(ThreadInput tin);
		void add_thread_output(ThreadOutput tout);
		void add_global_resource(void *addr, Reference resource);
	} context;
	
	void apply_group_allocation_map(const group_allocation_map &map);
	void apply_push_constant_allocation_map(const push_constant_allocation_map &map);

	template <typename T>
	Reference add(const T &sub, Debug aux);

	std::string repr() const {
		return "Block";
	}
};

extern template Reference Block::add <Argument> (const Argument &sub, Debug aux);
extern template Reference Block::add <Block> (const Block &sub, Debug aux);
extern template Reference Block::add <BuiltinIntrinsic> (const BuiltinIntrinsic &sub, Debug aux);
extern template Reference Block::add <Constant> (const Constant &sub, Debug aux);
extern template Reference Block::add <Construct> (const Construct &sub, Debug aux);
extern template Reference Block::add <Invocation> (const Invocation &sub, Debug aux);
extern template Reference Block::add <ArrayAccess> (const ArrayAccess &sub, Debug aux);
extern template Reference Block::add <FieldAccess> (const FieldAccess &sub, Debug aux);
extern template Reference Block::add <GlobalIntrinsic> (const GlobalIntrinsic &sub, Debug aux);
extern template Reference Block::add <GlobalResource> (const GlobalResource &sub, Debug aux);
extern template Reference Block::add <Branch> (const Branch &sub, Debug aux);
extern template Reference Block::add <Loop> (const Loop &sub, Debug aux);
extern template Reference Block::add <Local> (const Local &sub, Debug aux);
extern template Reference Block::add <Operation> (const Operation &sub, Debug aux);
extern template Reference Block::add <Store> (const Store &sub, Debug aux);
extern template Reference Block::add <Swizzle> (const Swizzle &sub, Debug aux);
extern template Reference Block::add <ThreadInput> (const ThreadInput &sub, Debug aux);
extern template Reference Block::add <ThreadOutput> (const ThreadOutput &sub, Debug aux);
extern template Reference Block::add <Type> (const Type &sub, Debug aux);
