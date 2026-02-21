#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include <memory>

#include "../util/variant.hpp"
#include "enumerations.hpp"

namespace rcgp {

// Forward declarations
struct Block;
struct Instruction;

using SharedBlockReference = std::shared_ptr <Block>;
using Reference = std::shared_ptr <Instruction>;

struct Struct : std::vector <Reference> {
	std::string name;
	std::vector <std::string> fields;
};

struct Array {
	Reference base;
	int64_t size;
};

// @node Type
struct Type : variant <Primitive, Struct, Array> {
	using variant_self::variant;

	std::string repr() const;
};

// @node Constant
struct Constant : variant <
	bool,
	int32_t,
	uint32_t,
	float,
	std::string
> {
	using variant_self::variant;

	std::string repr() const;
};

// @node Operation
struct Operation {
	OperationCode code;
	Reference a;
	Reference b;

	std::string repr() const;
};

struct Construct {
	Reference type;
	std::vector <Reference> args;

	std::string repr() const;
};

struct Invocation {
	SharedBlockReference sbr;
	std::vector <Reference> args;
	std::vector <Reference> returns;

	std::string repr() const;
};

struct Branch {
	struct Segment {
		Reference cond;
		SharedBlockReference body;
	};

	std::vector <Segment> segments;
	std::optional <SharedBlockReference> fallback;

	std::string repr() const;
};

struct Loop {
	SharedBlockReference body;

	std::string repr() const;
};

struct Local {
	Reference type;
	Reference init = nullptr;

	std::string repr() const;
};

struct BuiltinIntrinsic {
	BuiltinIntrinsicCode code;
	std::vector <Reference> args;

	std::string repr() const;
};

struct Swizzle {
	SwizzleCode code;
	Reference value;

	std::string repr() const;
};

struct FieldAccess {
	Reference value;
	uint32_t fidx;

	std::string repr() const;
};

struct ArrayAccess {
	Reference value;
	Reference index;

	std::string repr() const;
};

struct Store {
	Reference destination;
	Reference source;

	std::string repr() const;
};

struct GlobalResource {
	Reference type;
	GlobalResourceKind kind;
	GlobalResourceLayout layout;
	GlobalResourceAccess access = GlobalResourceAccess::eReadWrite;

	std::optional <uint32_t> group;
	std::optional <uint32_t> index;
	std::optional <uint32_t> offset;

	std::string repr() const;
};

struct Argument {
	Reference type;
	uint32_t argi;

	std::string repr() const;
};

struct StageInput {
	Reference type;
	uint32_t argi;
	RateProperties properties;

	std::string repr() const;
};

struct StageOutput {
	Reference type;
	uint32_t argi;
	RateProperties properties;

	std::string repr() const;
};

struct Return {
	Reference type;
	uint32_t argi;

	std::string repr() const;
};

// Additional methods
const Struct &get_struct(const SharedBlockReference &sbr, const Reference &ref);

} // namespace rcgp
