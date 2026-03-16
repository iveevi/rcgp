#pragma once

#include <source_location>

#include "tracer.hpp"

#define $location const std::source_location &loc = std::source_location::current()

namespace rcgp::jems {

struct scope {
	scope(const SharedBlockReference &sbr);
	~scope();

	operator bool() const;
};

struct null {
	void override_reference(const Reference &ref);
};

class handle {
protected:
	Reference _ref;
public:
	handle(const Reference &ref_ = nullptr);

	void override_reference(const Reference &ref_);

	operator Reference &();
	operator const Reference &() const;
};

// TODO: just make these all functions...
struct operation : handle {
	operation(
		OperationCode code,
		const Reference &a,
		const Reference &b = {},
		$location
	);
};

struct constant : handle {
	constant(const Constant &value, $location);
};

struct invocation : handle {
	invocation(
		const SharedBlockReference &sbr,
		const std::vector <Reference> &args,
		const std::vector <Reference> &returns,
		$location
	);
};

struct construct : handle {
	construct(
		const Reference &type,
		const std::vector <Reference> &args,
		$location
	);
};

struct argument : handle {
	argument(
		const Reference &type,
		uint32_t argi,
		$location
	);
};

struct returns : handle {
	returns(
		const Reference &type,
		uint32_t argi,
		$location
	);
};

struct stage_input : handle {
	stage_input(
		const Reference &type,
		uint32_t argi,
		RateProperties properties = {},
		$location
	);
};

struct stage_output : handle {
	stage_output(
		const Reference &type,
		uint32_t argi,
		RateProperties properties = {},
		$location
	);
};

struct global_resource : handle {
	global_resource(
		const Reference &type,
		GlobalResourceKind kind,
		GlobalResourceLayout layout,
		GlobalResourceAccess access = GlobalResourceAccess::eReadWrite,
		std::optional <uint32_t> group = std::nullopt,
		std::optional <uint32_t> index = std::nullopt,
		std::optional <uint32_t> offset = std::nullopt,
		$location
	);
};

struct system_value : handle {
	system_value(SystemValue value, $location);
};

struct builtin_intrinsic : handle {
	builtin_intrinsic(
		BuiltinIntrinsicCode code,
		const std::vector <Reference> &args = {},
		$location
	);
};

struct swizzle : handle {
	swizzle(
		SwizzleCode code,
		const Reference &value,
		$location
	);
};

struct array_access : handle {
	array_access(
		const Reference &value,
		const Reference &index,
		$location
	);
};

struct field_access : handle {
	field_access(
		const Reference &value,
		uint32_t fidx,
		$location
	);
};

struct store : handle {
	store(
		const Reference &destination,
		const Reference &source,
		$location
	);
};

struct local : handle {
	local(const Reference &type, $location);
};

struct loop : handle {
	loop(const SharedBlockReference &body, $location);
};

struct type : handle {
	type(const Type &t, $location);
};

} // namespace rcgp::jems

#define $break ::rcgp::jems::builtin_intrinsic { ::rcgp::BuiltinIntrinsicCode::eBreak }
#define $continue ::rcgp::jems::builtin_intrinsic { ::rcgp::BuiltinIntrinsicCode::eContinue }
#define $discard ::rcgp::jems::builtin_intrinsic { ::rcgp::BuiltinIntrinsicCode::eDiscard }
