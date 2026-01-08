#pragma once

#include "tracer.hpp"
#include "instructions.hpp"

// JIT Emitters
namespace jems {

class handle {
protected:
	Reference ref;
public:
	handle(const Reference &ref_ = nullptr) : ref(ref_) {}

	void override_reference(const Reference &ref_) {
		ref = ref_;
	}

	operator Reference &() {
		return ref;
	}
	
	operator const Reference &() const {
		return ref;
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
JEM(type, Type);
JEM(construct, Construct);
JEM(argument, Argument);
JEM(thread_input, ThreadInput);
JEM(thread_output, ThreadOutput);
JEM(global_resource, GlobalResource);
JEM(global_intrinsic, GlobalIntrinsic);
JEM(builtin_intrinsic, BuiltinIntrinsic);
JEM(swizzle, Swizzle);
JEM(array_access, ArrayAccess);
JEM(field_access, FieldAccess);
JEM(store, Store);

} // namespace jems

#define $location const std::source_location &loc = std::source_location::current()
