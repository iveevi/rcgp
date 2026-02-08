#pragma once

#include <algorithm>

#include "instructions.hpp"
#include "tracer.hpp"

namespace rcgp::jems {

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

#define JEM(name, type)	\
	template <typename ... Args>	\
	struct name : handle {	\
		name(Args ... args, const std::source_location &loc = std::source_location::current())	\
			: handle(Tracer::singleton.active().add(type(args...), DebugInfo(loc))) {}	\
	};	\
	template <typename ... Args>	\
	struct name##_loc : handle {	\
		name##_loc (const std::source_location &loc, Args ... args) \
			: handle(Tracer::singleton.active().add(type(args...), DebugInfo(loc))) {}	\
	};	\
	template <typename ... Args>	\
	name(Args ...) -> name <Args...>;

JEM(operation, Operation);
JEM(constant, Constant);
JEM(invocation, Invocation);
JEM(construct, Construct);
JEM(argument, Argument);
JEM(returns, Return);
JEM(stage_input, StageInput);
JEM(stage_output, StageOutput);
JEM(global_resource, GlobalResource);
JEM(system_value, SystemValue);
JEM(builtin_intrinsic, BuiltinIntrinsic);
JEM(swizzle, Swizzle);
JEM(array_access, ArrayAccess);
JEM(field_access, FieldAccess);
JEM(store, Store);
JEM(local, Local);
JEM(loop, Loop);

template <typename ... Args>
struct type : handle {
	type(Args ... args, const std::source_location &loc = std::source_location::current()) {
		static_assert(sizeof...(Args) == 1);
		Type t(args...);
		auto key = t.repr();
		auto &cache = Tracer::singleton.type_cache;
		if (auto it = cache.find(key); it != cache.end()) {
			_ref = it->second;
			auto &blk = Tracer::singleton.active();
			if (std::find(blk.begin(), blk.end(), _ref) == blk.end())
				blk.insert(blk.begin(), _ref);
			return;
		}
		_ref = Tracer::singleton.active().add(t, DebugInfo(loc));
		cache.emplace(std::move(key), _ref);
	}
};

template <typename ... Args>
struct type_loc : handle {
	type_loc(const std::source_location &loc, Args ... args) {
		static_assert(sizeof...(Args) == 1);
		Type t(args...);
		auto key = t.repr();
		auto &cache = Tracer::singleton.type_cache;
		if (auto it = cache.find(key); it != cache.end()) {
			_ref = it->second;
			auto &blk = Tracer::singleton.active();
			if (std::find(blk.begin(), blk.end(), _ref) == blk.end())
				blk.insert(blk.begin(), _ref);
			return;
		}
		_ref = Tracer::singleton.active().add(t, DebugInfo(loc));
		cache.emplace(std::move(key), _ref);
	}
};

template <typename ... Args>
type(Args ...) -> type <Args...>;

} // namespace rcgp::jems

#define $location const std::source_location &loc = std::source_location::current()

#define $break ::rcgp::jems::builtin_intrinsic { ::rcgp::BuiltinIntrinsicCode::eBreak }
#define $continue ::rcgp::jems::builtin_intrinsic { ::rcgp::BuiltinIntrinsicCode::eContinue }
#define $discard ::rcgp::jems::builtin_intrinsic { ::rcgp::BuiltinIntrinsicCode::eDiscard }
