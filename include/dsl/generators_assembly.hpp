#pragma once

#include <cstdint>
#include <map>

#include "dsl/instructions.hpp"

namespace generators {

struct Assembly {
	const SharedBlockReference &sbr;

	std::map <intptr_t, uint32_t> ids;

	Assembly(const SharedBlockReference &sbr_)
		: sbr(sbr_) {}

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
	std::string stringify(Invocation x, Reference ref);
	std::string stringify(GlobalIntrinsic x, Reference ref);
	std::string stringify(Construct x, Reference ref);
	std::string stringify(BuiltinIntrinsic x, Reference ref);
	std::string stringify(Swizzle x, Reference ref);
	std::string stringify(Branch x, Reference ref);
	std::string stringify(Loop x, Reference ref);
	std::string stringify(Local x, Reference ref);
	std::string stringify(Block x, Reference ref);
	std::string stringify(ShaderStage stage);
	std::string generate(size_t tabs = 0, bool emit_branches = true);

private:
	std::string stringify_block_ref(const SharedBlockReference &blk);
	std::string generate_block_body(const SharedBlockReference &blk, const std::string &indent);
};

} // namespace generators
