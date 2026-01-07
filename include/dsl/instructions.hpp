#pragma once

#include <map>
#include <memory>
#include <optional>
#include <set>
#include <source_location>
#include <string>
#include <vector>

#include <fmt/format.h>

#include "../util/variant.hpp"
#include "instruction_enums.hpp"

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
	using Code = OperationCode;
	Code code;

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
	using Kind = GlobalResourceKind;
	using Layout = GlobalResourceLayout;
	Kind kind;
	Layout layout;

	// group := descriptor set index
	std::optional <uint32_t> group;
	// index := descriptor binding
	std::optional <uint32_t> index;
	// push constants use this to disambiguate multiple blocks
	std::optional <uint32_t> push_constant_index;
	// push constants use this to determine offset within the shared block
	std::optional <uint32_t> push_constant_offset;
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
	using Code = BuiltinIntrinsicCode;
	Code code;

	std::vector <Reference> args;
	
	template <typename ... Args>
	BuiltinIntrinsic(Code code_, Args ... args_)
		: code(code_), args { args_ ... } {}
};

struct Swizzle {
	using Code = SwizzleCode;
	Code code;

	Reference value;
};

std::string swizzle_string(SwizzleCode code);

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

using group_allocation_map = std::map <void *, size_t>;

struct PushConstantAllocation {
	uint32_t index;
	uint32_t offset;
};

using push_constant_allocation_map = std::map <void *, PushConstantAllocation>;

struct Block : std::vector <Reference> {
	struct Context {
		ExecutionModel model = ExecutionModel::eAgnostic;

		std::vector <Argument> arguments;
		std::vector <ThreadInput> thread_inputs;
		std::vector <ThreadOutput> thread_outputs;
		std::map <void *, std::set <Reference>> global_resources;
		
		void add_argument(Argument arg);
		void add_thread_input(ThreadInput tin);
		void add_thread_output(ThreadOutput tout);

		template <auto &rsrc>
		void add_global_resource(Reference resource) {
			auto addr = (void *) &rsrc;
			if (not global_resources.contains(addr))
				global_resources.emplace(addr, std::set <Reference> ());

			global_resources[addr].insert(resource);
		}
	} context;
	
	void apply_group_allocation_map(const group_allocation_map &map);
	void apply_push_constant_allocation_map(const push_constant_allocation_map &map);

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

template <typename T>
Reference Block::add(const T &sub, const Debug aux)
{
	auto result = std::make_shared <Instruction> (sub, aux);
	emplace_back(result);
	return result;
}
