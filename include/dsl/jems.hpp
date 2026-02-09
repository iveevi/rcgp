#pragma once

#include <algorithm>

#include "instructions.hpp"
#include "tracer.hpp"

#define $location const std::source_location &loc = std::source_location::current()

namespace rcgp::jems {

struct scope {
	scope(const SharedBlockReference &sbr) {
		Tracer::singleton.records.emplace(sbr);
	}

	~scope() {
		Tracer::singleton.records.pop();
	}

	operator bool() const {
		return true;
	}
};

struct null {
	void override_reference(const Reference &ref) {}
};

class handle {
protected:
	Reference _ref;
public:
	handle(const Reference &ref_ = nullptr) : _ref(ref_) {}

	void override_reference(const Reference &ref_) {
		_ref = ref_;
	}

	operator Reference &() {
		return _ref;
	}
	
	operator const Reference &() const {
		return _ref;
	}
};

struct operation : handle {
	operation(
		OperationCode code,
		const Reference &a,
		const Reference &b = {},
		$location
	) : handle($tsb.add(Instruction(Operation { code, a, b }, DebugInfo(loc)))) {}
};

struct constant : handle {
	constant(const Constant &value, $location)
		: handle($tsb.add(Instruction(value, DebugInfo(loc)))) {}
};

struct invocation : handle {
	invocation(
		const SharedBlockReference &sbr,
		const std::vector <Reference> &args,
		const std::vector <Reference> &returns,
		$location
	) : handle($tsb.add(Instruction(Invocation { sbr, args, returns }, DebugInfo(loc)))) {}
};

struct construct : handle {
	construct(
		const Reference &type,
		const std::vector <Reference> &args,
		$location
	) : handle($tsb.add(Instruction(Construct { type, args }, DebugInfo(loc)))) {}
};

struct argument : handle {
	argument(
		const Reference &type,
		uint32_t argi,
		$location
	) : handle($tsb.add(Instruction(Argument { type, argi }, DebugInfo(loc)))) {}
};

struct returns : handle {
	returns(
		const Reference &type,
		uint32_t argi,
		$location
	) : handle($tsb.add(Instruction(Return { type, argi }, DebugInfo(loc)))) {}
};

struct stage_input : handle {
	stage_input(
		const Reference &type,
		uint32_t argi,
		RateProperties properties = {},
		$location
	) : handle($tsb.add(Instruction(StageInput { type, argi, properties }, DebugInfo(loc)))) {}
};

struct stage_output : handle {
	stage_output(
		const Reference &type,
		uint32_t argi,
		RateProperties properties = {},
		$location
	) : handle($tsb.add(Instruction(StageOutput { type, argi, properties }, DebugInfo(loc)))) {}
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
	) : handle($tsb.add(Instruction(
		GlobalResource { type, kind, layout, access, group, index, offset },
		DebugInfo(loc)
	))) {}
};

struct system_value : handle {
	system_value(SystemValue value, $location)
		: handle($tsb.add(Instruction(value, DebugInfo(loc)))) {}
};

struct builtin_intrinsic : handle {
	builtin_intrinsic(
		BuiltinIntrinsicCode code,
		const std::vector <Reference> &args = {},
		$location
	) : handle($tsb.add(Instruction(BuiltinIntrinsic { code, args }, DebugInfo(loc)))) {}
};

struct swizzle : handle {
	swizzle(
		SwizzleCode code,
		const Reference &value,
		$location
	) : handle($tsb.add(Instruction(Swizzle { code, value }, DebugInfo(loc)))) {}
};

struct array_access : handle {
	array_access(
		const Reference &value,
		const Reference &index,
		$location
	) : handle($tsb.add(Instruction(ArrayAccess { value, index }, DebugInfo(loc)))) {}
};

struct field_access : handle {
	field_access(
		const Reference &value,
		uint32_t fidx,
		$location
	) : handle($tsb.add(Instruction(FieldAccess { value, fidx }, DebugInfo(loc)))) {}
};

struct store : handle {
	store(
		const Reference &destination,
		const Reference &source,
		$location
	) : handle($tsb.add(Instruction(Store { destination, source }, DebugInfo(loc)))) {}
};

struct local : handle {
	local(const Reference &type, $location)
		: handle($tsb.add(Instruction(Local { type }, DebugInfo(loc)))) {}
};

struct loop : handle {
	loop(const SharedBlockReference &body, $location)
		: handle($tsb.add(Instruction(Loop { body }, DebugInfo(loc)))) {}
};

struct type : handle {
	type(const Type &t, $location) {
		auto key = t.repr();
		// TODO: per block type cache
		auto &cache = Tracer::singleton.type_cache;
		if (auto it = cache.find(key); it != cache.end()) {
			_ref = it->second;
			auto &blk = Tracer::singleton.active();
			if (std::find(blk.begin(), blk.end(), _ref) == blk.end())
				blk.insert(blk.begin(), _ref);
			return;
		}
		_ref = $tsb.add(Instruction(t, DebugInfo(loc)));
		cache.emplace(key, _ref);
	}
};

} // namespace rcgp::jems

#define $break ::rcgp::jems::builtin_intrinsic { ::rcgp::BuiltinIntrinsicCode::eBreak }
#define $continue ::rcgp::jems::builtin_intrinsic { ::rcgp::BuiltinIntrinsicCode::eContinue }
#define $discard ::rcgp::jems::builtin_intrinsic { ::rcgp::BuiltinIntrinsicCode::eDiscard }
