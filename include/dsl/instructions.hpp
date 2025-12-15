#pragma once

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <source_location>

#include <fmt/format.h>

#include "../util/variant.hpp"

struct Instruction;

// TODO: need a better (paged arena) allocator which effectively just uint32_t or smth
using Reference = std::shared_ptr <Instruction>;

struct Constant : variant <
	bool,
	int32_t,
	uint32_t,
	float, std::string
> {
	using variant_self::variant;
};

struct Operation {
	enum Code {
		eAdd,
		eSubtract,
		eMultiply,
		eDivide,
	} code;

	Reference a;
	Reference b;
};

template <typename T, size_t N>
struct VectorType {};

template <typename T, size_t N, size_t M>
struct MatrixType {};

struct PrimitiveType : variant <
	bool,
	int32_t,
	uint32_t,
	float,
	// Vector types
	VectorType <uint32_t, 1>, // mainly symbolic
	VectorType <uint32_t, 2>,
	VectorType <uint32_t, 3>,
	VectorType <uint32_t, 4>,

	VectorType <int32_t, 1>,
	VectorType <int32_t, 2>,
	VectorType <int32_t, 3>,
	VectorType <int32_t, 4>,

	VectorType <float, 1>,
	VectorType <float, 2>,
	VectorType <float, 3>,
	VectorType <float, 4>,
	// Matrix types
	MatrixType <float, 3, 3>,
	MatrixType <float, 4, 4>
> {
	using variant_self::variant;
};

struct AggregateType : std::vector <Reference> {};

struct ArrayType {
	Reference base;
	int64_t size;
};

struct Type : variant <
	PrimitiveType,
	AggregateType,
	ArrayType
> {
	using variant_self::variant;
};

struct Construct {
	Reference type;
	std::vector <Reference> args;

	template <typename ... Args>
	Construct(Reference type_, Args ... args_)
		: type(type_), args { args_ ... } {}
};

struct Argument {
	Reference type;
	uint32_t argi;
};

struct GlobalResource {
	Reference type;

	enum Kind {
		ePushConstant,
		eUniformBuffer,
		eStorageBuffer,
		eSampler,
	} kind;

	enum class Layout {
		eUnknown,
		eScalar,
		eStd430,
	} layout;

	// group := descriptor set index
	std::optional <uint32_t> group;
	// index := descriptor binding
	std::optional <uint32_t> index;
};

enum class GlobalIntrinsic {
	eScreenPosition,
	eInstanceIndex,
	eVertexIndex,
};

enum class RateProperties {
	eNone,
	eSmooth,
	eFlat,
	eNoPerspective,
};

struct ThreadInput {
	Reference type;
	// corresponds to actual order
        uint32_t argi;
};

struct ThreadOutput {
	Reference type;
	uint32_t argi;

	RateProperties properties;
};

struct BuiltinIntrinsic {
	enum Code {
		eCos,
		eDot,
		eMax,
		eMin,
		eNormalize,
		eTranspose,
		eInverse,
		eSample,
		eSin,
		eTan,
	} code;

	std::vector <Reference> args;
	
	template <typename ... Args>
	BuiltinIntrinsic(Code code_, Args ... args_)
		: code(code_), args { args_ ... } {}
};

struct Swizzle {
	enum Code {
		// Level 1
		eX, eY, eZ, eW,

		// Level 2
		eXX, eXY, eXZ, eXW,
		eYX, eYY, eYZ, eYW,
		eZX, eZY, eZZ, eZW,
		eWX, eWY, eWZ, eWW,
		
		// Level 3
		eXXX, eXXY, eXXZ, eXXW, eXYX, eXYY, eXYZ, eXYW, eXZX, eXZY, eXZZ, eXZW, eXWX, eXWY, eXWZ, eXWW,
		eYXX, eYXY, eYXZ, eYXW, eYYX, eYYY, eYYZ, eYYW, eYZX, eYZY, eYZZ, eYZW, eYWX, eYWY, eYWZ, eYWW,
		eZXX, eZXY, eZXZ, eZXW, eZYX, eZYY, eZYZ, eZYW, eZZX, eZZY, eZZZ, eZZW, eZWX, eZWY, eZWZ, eZWW,
		eWXX, eWXY, eWXZ, eWXW, eWYX, eWYY, eWYZ, eWYW, eWZX, eWZY, eWZZ, eWZW, eWWX, eWWY, eWWZ, eWWW,
		
		// Level 4
		eXXXX, eXXXY, eXXXZ, eXXXW, eXXYX, eXXYY, eXXYZ, eXXYW, eXXZX, eXXZY, eXXZZ, eXXZW, eXXWX, eXXWY, eXXWZ, eXXWW,
		eXYXX, eXYXY, eXYXZ, eXYXW, eXYYX, eXYYY, eXYYZ, eXYYW, eXYZX, eXYZY, eXYZZ, eXYZW, eXYWX, eXYWY, eXYWZ, eXYWW,
		eXZXX, eXZXY, eXZXZ, eXZXW, eXZYX, eXZYY, eXZYZ, eXZYW, eXZZX, eXZZY, eXZZZ, eXZZW, eXZWX, eXZWY, eXZWZ, eXZWW,
		eXWXX, eXWXY, eXWXZ, eXWXW, eXWYX, eXWYY, eXWYZ, eXWYW, eXWZX, eXWZY, eXWZZ, eXWZW, eXWWX, eXWWY, eXWWZ, eXWWW,
		
		eYXXX, eYXXY, eYXXZ, eYXXW, eYXYX, eYXYY, eYXYZ, eYXYW, eYXZX, eYXZY, eYXZZ, eYXZW, eYXWX, eYXWY, eYXWZ, eYXWW,
		eYYXX, eYYXY, eYYXZ, eYYXW, eYYYX, eYYYY, eYYYZ, eYYYW, eYYZX, eYYZY, eYYZZ, eYYZW, eYYWX, eYYWY, eYYWZ, eYYWW,
		eYZXX, eYZXY, eYZXZ, eYZXW, eYZYX, eYZYY, eYZYZ, eYZYW, eYZZX, eYZZY, eYZZZ, eYZZW, eYZWX, eYZWY, eYZWZ, eYZWW,
		eYWXX, eYWXY, eYWXZ, eYWXW, eYWYX, eYWYY, eYWYZ, eYWYW, eYWZX, eYWZY, eYWZZ, eYWZW, eYWWX, eYWWY, eYWWZ, eYWWW,
		
		eZXXX, eZXXY, eZXXZ, eZXXW, eZXYX, eZXYY, eZXYZ, eZXYW, eZXZX, eZXZY, eZXZZ, eZXZW, eZXWX, eZXWY, eZXWZ, eZXWW,
		eZYXX, eZYXY, eZYXZ, eZYXW, eZYYX, eZYYY, eZYYZ, eZYYW, eZYZX, eZYZY, eZYZZ, eZYZW, eZYWX, eZYWY, eZYWZ, eZYWW,
		eZZXX, eZZXY, eZZXZ, eZZXW, eZZYX, eZZYY, eZZYZ, eZZYW, eZZZX, eZZZY, eZZZZ, eZZZW, eZZWX, eZZWY, eZZWZ, eZZWW,
		eZWXX, eZWXY, eZWXZ, eZWXW, eZWYX, eZWYY, eZWYZ, eZWYW, eZWZX, eZWZY, eZWZZ, eZWZW, eZWWX, eZWWY, eZWWZ, eZWWW,
		
		eWXXX, eWXXY, eWXXZ, eWXXW, eWXYX, eWXYY, eWXYZ, eWXYW, eWXZX, eWXZY, eWXZZ, eWXZW, eWXWX, eWXWY, eWXWZ, eWXWW,
		eWYXX, eWYXY, eWYXZ, eWYXW, eWYYX, eWYYY, eWYYZ, eWYYW, eWYZX, eWYZY, eWYZZ, eWYZW, eWYWX, eWYWY, eWYWZ, eWYWW,
		eWZXX, eWZXY, eWZXZ, eWZXW, eWZYX, eWZYY, eWZYZ, eWZYW, eWZZX, eWZZY, eWZZZ, eWZZW, eWZWX, eWZWY, eWZWZ, eWZWW,
		eWWXX, eWWXY, eWWXZ, eWWXW, eWWYX, eWWYY, eWWYZ, eWWYW, eWWZX, eWWZY, eWWZZ, eWWZW, eWWWX, eWWWY, eWWWZ, eWWWW,
	} code;

	Reference value;
};

struct FieldAccess {
	Reference value;
	uint32_t fidx;
};

struct ArrayAccess {
	Reference value;
	Reference index;
};

struct Store {
	Reference destination;
	Reference source;
};

// TODO: enable/disable with macros... or templated instructions?
struct Debug {
	std::source_location origin;
};

struct Return {
	Reference value;
};

enum class ExecutionModel {
	eAgnostic,
	eVulkanVertex,
	eVulkanFragment,
	eVulkanCompute,
};

using group_allocation_map = std::map <void *, size_t>;

struct Block : std::vector <Reference> {
	struct Context {
		ExecutionModel model = ExecutionModel::eAgnostic;

		std::vector <Argument> arguments;
		std::vector <ThreadInput> thread_inputs;
		std::vector <ThreadOutput> thread_outputs;
		std::map <void *, std::set <Reference>> global_resources;
		
		void add_argument(Argument arg) {
			if (arguments.size() > arg.argi) {
				// already registered
				__builtin_trap();
			} else {
				arguments.resize(arg.argi + 1);
				arguments[arg.argi] = arg;
			}
		}

		void add_thread_input(ThreadInput tin) {
			if (thread_inputs.size() > tin.argi) {
				// already registered
				__builtin_trap();
			} else {
				thread_inputs.resize(tin.argi + 1);
				thread_inputs[tin.argi] = tin;
			}
		}
		
		void add_thread_output(ThreadOutput tout) {
			if (thread_outputs.size() > tout.argi) {
				// already registered
				// TODO: this is fine, just make sure its the same or
				// its uninitialized...
				__builtin_trap();
			} else {
				thread_outputs.resize(tout.argi + 1);
				thread_outputs[tout.argi] = tout;
			}
		}

		template <auto &rsrc>
		void add_global_resource(Reference resource) {
			auto addr = (void *) &rsrc;
			if (not global_resources.contains(addr))
				global_resources.emplace(addr, std::set <Reference> ());

			global_resources[addr].insert(resource);
		}
	} context;
	
	void apply_group_allocation_map(const group_allocation_map &map);

	template <typename T>
	Reference add(const T &sub, Debug aux);
};

struct Instruction : variant <
	Argument,
	Block,
	BuiltinIntrinsic,
	Constant,
	Construct,
	ArrayAccess,
	FieldAccess,
	GlobalIntrinsic,
	GlobalResource,
	Operation,
	Store,
	Swizzle,
	ThreadInput,
	ThreadOutput,
	Type
> {
	Debug debug_info;

	Instruction(const variant_self &base, Debug debug_info_ = {})
		: variant_self(base), debug_info(debug_info_) {}
};

inline void Block::apply_group_allocation_map(const group_allocation_map &map)
{
	for (auto &[addr, group] : map) {
		auto &refs = context.global_resources[addr];
		for (auto &ref : refs)
			ref->as <GlobalResource> ().group = group;
	}
}

template <typename T>
Reference Block::add(const T &sub, const Debug aux)
{
	auto result = std::make_shared <Instruction> (sub, aux);
	emplace_back(result);
	return result;
}
