#pragma once

#include <algorithm>

#include "tracer.hpp"
#include "instructions.hpp"

namespace rcgp {

// JIT Emitters
namespace jems {

struct handle {
	Reference _ref;

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

	operator bool() const {
		return _ref.get() != nullptr;
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
			: handle(Tracer::singleton.active().add(type(args...), Debug(loc))) {}	\
	};	\
	template <typename ... Args>	\
	struct name##_loc : handle {	\
		name##_loc (const std::source_location &loc, Args ... args) \
			: handle(Tracer::singleton.active().add(type(args...), Debug(loc))) {}	\
	};	\
	template <typename ... Args>	\
	name(Args ...) -> name <Args...>;

JEM(operation, Operation);
JEM(constant, Constant);
JEM(invocation, Invocation);
JEM(construct, Construct);
JEM(argument, Argument);
JEM(stage_input, StageInput);
JEM(stage_output, StageOutput);
JEM(global_resource, GlobalResource);
JEM(global_intrinsic, GlobalIntrinsic);
JEM(builtin_intrinsic, BuiltinIntrinsic);
JEM(swizzle, Swizzle);
JEM(array_access, ArrayAccess);
JEM(field_access, FieldAccess);
JEM(store, Store);
JEM(local, Local);

// TODO: separate header & source later...
inline size_t type_cache_key_for(const Type &type)
{
	auto hash_combine = [](size_t &seed, size_t v) {
		seed ^= v + 0x9e3779b97f4a7c15ull + (seed << 6) + (seed >> 2);
	};
	auto hash_string = [&](size_t &seed, const std::string &value) {
		for (unsigned char c : value)
			hash_combine(seed, c);
	};

	size_t key = 0;
	vswitch (type) {
	vcase(PrimitiveType): {
		auto &prim = type.as <PrimitiveType> ();
		key = RuntimeTypeRegistry::id <PrimitiveType> ();
		hash_combine(key, prim.index());
		return key;
	}
	vcase(AggregateType): {
		auto &agg = type.as <AggregateType> ();
		key = RuntimeTypeRegistry::id <AggregateType> ();
		hash_string(key, agg.name);
		hash_combine(key, agg.size());
		for (auto &field : agg)
			hash_combine(key, reinterpret_cast <size_t> (field.get()));
		return key;
	}
	vcase(ArrayType): {
		auto &arr = type.as <ArrayType> ();
		key = RuntimeTypeRegistry::id <ArrayType> ();
		hash_combine(key, reinterpret_cast <size_t> (arr.base.get()));
		hash_combine(key, static_cast <size_t> (arr.size));
		return key;
	}
	default:
		break;
	}
	return key;
}

template <typename ... Args>
struct type : handle {
	type(Args ... args, const std::source_location &loc = std::source_location::current()) {
		Type t(args...);
		auto key = type_cache_key_for(t);
		auto &cache = Tracer::singleton.type_cache;
		if (auto it = cache.find(key); it != cache.end()) {
			_ref = it->second;
			auto &blk = Tracer::singleton.active();
			if (std::find(blk.begin(), blk.end(), _ref) == blk.end())
				blk.insert(blk.begin(), _ref);
			return;
		}
		_ref = Tracer::singleton.active().add(t, Debug(loc));
		cache.emplace(std::move(key), _ref);
	}
};

template <typename ... Args>
struct type_loc : handle {
	type_loc(const std::source_location &loc, Args ... args) {
		Type t(args...);
		auto key = type_cache_key_for(t);
		auto &cache = Tracer::singleton.type_cache;
		if (auto it = cache.find(key); it != cache.end()) {
			_ref = it->second;
			auto &blk = Tracer::singleton.active();
			if (std::find(blk.begin(), blk.end(), _ref) == blk.end())
				blk.insert(blk.begin(), _ref);
			return;
		}
		_ref = Tracer::singleton.active().add(t, Debug(loc));
		cache.emplace(std::move(key), _ref);
	}
};

template <typename ... Args>
type(Args ...) -> type <Args...>;

} // namespace jems
} // namespace rcgp

#define $location const std::source_location &loc = std::source_location::current()
