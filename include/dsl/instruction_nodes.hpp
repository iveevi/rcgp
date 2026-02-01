#pragma once

#include <cstdint>
#include <optional>
#include <source_location>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "instruction_enums.hpp"
#include "instruction_types.hpp"

namespace rcgp {

struct Constant : variant <
	bool,
	int32_t,
	uint32_t,
	float, std::string
> {
	using variant_self::variant;

	std::string repr() const {
		return "Constant";
	}
};

struct Operation {
	using Code = OperationCode;
	Code code;

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

struct Argument {
	Reference type;
	uint32_t argi;

	std::string repr() const {
		// TODO: display reference of self
		return "Argument";
	}
};

struct BuiltinIntrinsic {
	using Code = BuiltinIntrinsicCode;
	Code code;

	std::vector <Reference> args;
	
	template <typename ... Args>
	BuiltinIntrinsic(Code code_, Args ... args_)
		: code(code_), args { args_ ... } {}

	std::string repr() const {
		return "BuiltinIntrinsic";
	}
};

struct Swizzle {
	using Code = SwizzleCode;
	Code code;

	Reference value;

	std::string repr() const {
		return "Swizzle";
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
