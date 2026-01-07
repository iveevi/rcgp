#pragma once

#include <cstdint>
#include <map>

#include "dsl/instructions.hpp"

namespace generators {

struct Assembly {
	const Block &block;

	std::map <std::intptr_t, uint32_t> ids;

	std::string stringify(Reference ref);
	std::string stringify(Constant x, Reference ref);
	std::string stringify_type(PrimitiveType x, Reference ref);
	std::string stringify_type(AggregateType x, Reference ref);
	std::string stringify_type(ArrayType x, Reference ref);
	std::string stringify(Type x, Reference ref);
	std::string stringify(Operation x, Reference ref);
	std::string stringify(Store x, Reference ref);
	std::string stringify(ArrayAccess x, Reference ref);
	std::string stringify(FieldAccess x, Reference ref);
	std::string stringify(Argument x, Reference ref);
	std::string stringify(GlobalResourceLayout layout);
	std::string stringify(GlobalResource x, Reference ref);
	std::string stringify(ThreadInput x, Reference ref);
	std::string stringify_rate_properties(RateProperties properties);
	std::string stringify(ThreadOutput x, Reference ref);
	std::string stringify(GlobalIntrinsic x, Reference ref);
	std::string stringify(Construct x, Reference ref);
	std::string stringify(BuiltinIntrinsic x, Reference ref);
	std::string stringify(Swizzle x, Reference ref);
	std::string stringify(Block x, Reference ref);
	std::string stringify(ExecutionModel model);
	std::string generate(size_t tabs = 0);
};

} // namespace generators
