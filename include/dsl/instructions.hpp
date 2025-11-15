#pragma once

#include <source_location>
#include <filesystem>

#include <fmt/format.h>

#include "../util/variant.hpp"

struct Instruction;

using Index = uint32_t;

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
	enum : uint32_t {
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
	MatrixType <float, 4, 4>
> {
	using variant_self::variant;
};

struct AggregateType : std::vector <Reference> {};

struct ArrayType {
	Reference base;
	Reference size;
};

struct Type : variant <
	PrimitiveType,
	AggregateType,
	ArrayType
> {
	using variant_self::variant;
};

struct invocable {
	Reference type;
	std::vector <Reference> args;

	template <typename ... Args>
	invocable(Reference type_, Args ... args_) : type(type_), args { args_ ... } {}
};

struct Construct : invocable {
	using invocable::invocable;
};

struct Argument {
	Reference type;
	uint32_t argi;
};

struct GlobalResource {
	Reference type;

	enum Kind {
		eXConstant,
		ePushConstant,
		eConstantBuffer,
		eStorageBuffer,
		eSampler,
	} kind;

	// group := descriptor set index
	std::optional <uint32_t> group;
	// index := descriptor binding
	std::optional <uint32_t> index;
};

enum class GlobalIntrinsic {
	eSVPosition,
};

struct ThreadInput {
	Reference type;
	// corresponds to actual order
        uint32_t argi;
};

struct ThreadOutput {
	Reference type;
	uint32_t argi;

	// TODO: fragment shader inputs also need this
	// (although they will be inferred by the order);
	// we should move this to RateProperties in the global scope
	enum Properties {
		eSmooth,
		eFlat,
		eNoPerspective,
	} properties;
};

struct Intrinsic : variant <
	Argument,
	ThreadInput,
	ThreadOutput,
	GlobalResource,
	GlobalIntrinsic
> {
	using variant_self::variant;
};

struct FieldAccess {
	Reference value;
	uint32_t fidx;
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

// TODO: this whole infrastructure can be reused for future compiler projects :)
struct Block : std::vector <Reference> {
	// TODO: parameters, type, and context information
	struct Context {
		ExecutionModel model = ExecutionModel::eAgnostic;

		std::vector <Argument> arguments;
		std::vector <ThreadInput> thread_inputs;
		std::vector <ThreadOutput> thread_outputs;
		
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
	} context;

	template <typename T>
	Reference add(const T &sub, Debug aux);
};

struct Instruction : variant <
	Constant,
	Operation,
	Type,
	Construct,
	Intrinsic,
	FieldAccess,
	Store,
	Block
> {
	Debug debug_info;

	Instruction(const variant_self &base, Debug debug_info_ = {})
		: variant_self(base), debug_info(debug_info_) {}
};

template <typename T>
Reference Block::add(const T &sub, const Debug aux)
{
	auto result = std::make_shared <Instruction> (sub, aux);
	emplace_back(result);
	return result;
}
