#pragma once

#include "instruction_block.hpp"
#include "instruction_nodes.hpp"
#include "instruction_resources.hpp"
#include "instruction_types.hpp"

struct Instruction : variant <
	Argument,
	ArrayAccess,
	Block,
	BuiltinIntrinsic,
	Constant,
	Construct,
	FieldAccess,
	GlobalIntrinsic,
	GlobalResource,
	Branch,
	Loop,
	Local,
	Invocation,
	Operation,
	Store,
	Swizzle,
	ThreadInput,
	ThreadOutput,
	Type
> {
	Debug debug_info;

	Instruction(const variant_self &base, Debug debug_info_ = {})
		: variant_self(base), debug_info(debug_info_) {}

	std::string repr() const {
		return std::visit([] <typename T> (T x) -> std::string {
			if constexpr (std::same_as <T, GlobalIntrinsic>)
				return "GlobalIntrinsic";
			else
				return x.repr();
		}, *this);
	}
};
