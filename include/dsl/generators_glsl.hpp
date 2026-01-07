#pragma once

#include "meta/static_string.hpp"
#include "util/logging.hpp"
#include "dsl/instructions.hpp"

#define DEFINE_CALL_OPERATOR(ReturnType) ReturnType operator()(Reference r);

namespace generators {

// TODO: compiler-like logging...
struct GLSL {
	const Block &block;

	std::vector <std::string> thread_inputs;

	// TODO: use cref?
	std::map <const AggregateType *, std::string> aggregate_names;

	// TODO: prelude section for types, etc.
	std::string result; // TODO: refactor to 'code'

	// TODO: indentation as well
	
	GLSL(const Block &block_);

	std::string layout_string(GlobalResourceLayout layout) const;
	
	struct generate_reference {
		GLSL &parent;

		std::string impl(auto x) {
			auto type_name = std::string($ss_type(decltype(x)).view());
			fatal("generate_reference::impl not implemented for %s", type_name.c_str());
		}

		std::string impl(GlobalIntrinsic gi);
		std::string impl(GlobalResource grsrc);
		std::string impl(ThreadOutput tout);
		
		std::string main(Reference reference);
		DEFINE_CALL_OPERATOR(std::string);
	} reference;

	struct generate_expression {
		GLSL &parent;

		std::string impl(auto x) {
			auto type_name = std::string($ss_type(decltype(x)).view());
			fatal("generate_expression::impl not implemented for %s", type_name.c_str());
		}
		
		std::string impl(Constant value);
		std::string impl(Operation operation);
		std::string impl(ThreadInput tin);
		std::string impl(Construct construct);
		std::string impl(FieldAccess access);
		std::string impl(ArrayAccess access);
		std::string impl(Swizzle swizzle);
		std::string impl(GlobalResource grsrc);
		std::string impl(GlobalIntrinsic intrinsic);
		std::string impl(const BuiltinIntrinsic &builtin);
		std::string main(Reference expression);
		DEFINE_CALL_OPERATOR(std::string);
	} expression;

	struct generate_statement {
		GLSL &parent;

		void impl(Store store);

		void impl(auto x) {
			parent.result += "    ?\n";
		}

		void main(Reference instruction);
		DEFINE_CALL_OPERATOR(void);
	} statement;

	struct generate_type {
		GLSL &parent;

		// TODO: options... (e.g. version)
		std::string impl(const PrimitiveType &primitive);
		std::string impl(const Type &type);
		std::string impl(const ArrayType &array);
		std::string impl(const AggregateType &aggregate);
		
		std::string impl(auto type) {
			auto type_name = std::string($ss_type(decltype(type)).view());
			fatal("generate_type::impl not implemented for %s", type_name.c_str());
		}
		
		std::string main(Reference type);
		DEFINE_CALL_OPERATOR(std::string);
	} type;

	std::string generate(size_t tabs = 0);
};

} // namespace generators

#undef DEFINE_CALL_OPERATOR
