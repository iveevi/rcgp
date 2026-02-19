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

	template <typename F>
	void apply(F &&ftn) {
		auto &&fn = ftn;

		vswitch (*this) {
		vcase(Argument):
			fn(as <Argument> ().type);
			break;
		vcase(ArrayAccess): {
			auto &aacc = as <ArrayAccess> ();
			fn(aacc.value);
			fn(aacc.index);
			break;
		}
		vcase(BuiltinIntrinsic): {
			auto &bintr = as <BuiltinIntrinsic> ();
			for (auto &arg : bintr.args)
				fn(arg);
			break;
		}
		vcase(Construct): {
			auto &ctor = as <Construct> ();
			fn(ctor.type);
			for (auto &arg : ctor.args)
				fn(arg);
			break;
		}
		vcase(FieldAccess):
			fn(as <FieldAccess> ().value);
			break;
		vcase(GlobalResource):
			fn(as <GlobalResource> ().type);
			break;
		vcase(Branch): {
			auto &branch = as <Branch> ();
			for (auto &segment : branch.segments)
				fn(segment.cond);
			break;
		}
		vcase(Local):
			fn(as <Local> ().type);
			break;
		vcase(Invocation): {
			auto &inv = as <Invocation> ();
			for (auto &arg : inv.args)
				fn(arg);
			for (auto &ret : inv.returns)
				fn(ret);
			break;
		}
		vcase(Operation): {
			auto &opn = as <Operation> ();
			fn(opn.a);
			fn(opn.b);
			break;
		}
		vcase(Store): {
			auto &store = as <Store> ();
			fn(store.destination);
			fn(store.source);
			break;
		}
		vcase(Swizzle):
			fn(as <Swizzle> ().value);
			break;
		vcase(StageInput):
			fn(as <StageInput> ().type);
			break;
		vcase(StageOutput):
			fn(as <StageOutput> ().type);
			break;
		vcase(Return):
			fn(as <Return> ().type);
			break;
		vcase(Type): {
			auto &type = as <Type> ();
			vswitch (type) {
			vcase(Struct): {
				auto &st = type.as <Struct> ();
				for (auto &field : st)
					fn(field);
				break;
			}
			vcase(Array):
				fn(type.as <Array> ().base);
				break;
			default:
				break;
			}
			break;
		}
		default:
			break;
		}
	}

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
