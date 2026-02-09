#pragma once

#include <source_location>

#include "instruction_nodes.hpp"

namespace rcgp {

// DebugInfo information
// TODO: just inline this...
struct DebugInfo {
	// TODO: node name
	std::source_location origin;
};

struct Instruction : variant <
	Argument,
	ArrayAccess,
	BuiltinIntrinsic,
	Constant,
	Construct,
	FieldAccess,
	SystemValue,
	GlobalResource,
	Branch,
	Loop,
	Local,
	Invocation,
	Operation,
	Store,
	Swizzle,
	StageInput,
	StageOutput,
	Return,
	Type
> {
	DebugInfo debug_info;

	Instruction(const variant_self &base, DebugInfo debug_info_ = {})
		: variant_self(base), debug_info(debug_info_) {}

	// TODO: move to source with vswitch and etc
	std::string repr() const {
		return std::visit([&] <typename T> (T x) -> std::string {
			if constexpr (std::same_as <T, SystemValue>)
				return std::string(rcgp::repr(this->as <SystemValue> ()));
			else
				return x.repr();
		}, *this);
	}
};

} // namespace rcgp
