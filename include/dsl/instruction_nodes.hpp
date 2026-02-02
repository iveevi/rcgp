#pragma once

#include <cstdint>
#include <optional>
#include <source_location>
#include <string>
#include <vector>
#include <print>

#include <fmt/format.h>

#include "instruction_enums.hpp"
#include "instruction_types.hpp"

namespace rcgp {

struct Constant : variant <
	bool,
	int32_t,
	uint32_t,
	float,
	std::string
> {
	using variant_self::variant;

	std::string repr() const {
		return std::visit([](auto x) {
			return fmt::format("{}", x);
		}, *this);
	}
};

struct Operation {
	OperationCode code;
	Reference a;
	Reference b;

	std::string repr() const {
		return fmt::format("Operation({})", rcgp::repr(code));
	}
};

struct Construct {
	Reference type;
	std::vector <Reference> args;

	template <typename ... Args>
	Construct(Reference type_, Args ... args_)
		: type(type_), args { args_ ... } {}

	std::string repr() const {
		return "Construct";
	}
};

struct Invocation {
	SharedBlockReference sbr;
	std::vector <Reference> args;

	template <typename ... Args>
	Invocation(SharedBlockReference sbr_, Args ... args_)
		: sbr(sbr_), args { args_ ... } {}

	std::string repr() const {
		return "Invocation";
	}
};

struct Branch {
	struct Segment {
		Reference cond;
		SharedBlockReference body;
	};

	std::vector <Segment> segments;
	std::optional <SharedBlockReference> fallback;

	std::string repr() const {
		return "Branch";
	}
};

struct Loop {
	LoopKind kind;
	std::optional <SharedBlockReference> init;
	SharedBlockReference cond;
	Reference cond_value;
	std::optional <SharedBlockReference> step;
	SharedBlockReference body;

	std::string repr() const {
		return "Loop";
	}
};

struct Local {
	Reference type;

	std::string repr() const {
		return "Local";
	}
};

struct BuiltinIntrinsic {
	BuiltinIntrinsicCode code;

	std::vector <Reference> args;
	
	template <typename ... Args>
	BuiltinIntrinsic(BuiltinIntrinsicCode code_, Args ... args_)
		: code(code_), args { args_ ... } {}

	std::string repr() const {
		return std::format("BuiltinIntrinsic({})", rcgp::repr(code));
	}
};

struct Swizzle {
	SwizzleCode code;
	Reference value;

	std::string repr() const {
		return std::format("Swizzle({})", rcgp::repr(code));
	}
};

struct FieldAccess {
	Reference value;
	uint32_t fidx;

	std::string repr() const {
		return "FieldAccess";
	}
};

struct ArrayAccess {
	Reference value;
	Reference index;

	std::string repr() const {
		return "ArrayAccess";
	}
};

struct Store {
	Reference destination;
	Reference source;

	std::string repr() const {
		return "Store";
	}
};

// TODO: enable/disable with macros... or templated instructions?
struct Debug {
	std::source_location origin;
};

struct Return {
	Reference value;

	std::string repr() const {
		return "Return";
	}
};

} // namespace rcgp
