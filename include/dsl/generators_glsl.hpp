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
	std::vector <std::string> argument_names;
	std::map <uint32_t, std::string> local_thread_outputs;
	std::map <const Instruction *, std::string> local_names;
	uint32_t local_count = 0;

	// TODO: use cref?
	std::map <const AggregateType *, std::string> aggregate_names;
	std::map <const Block *, std::string> subroutine_names;
	std::set <const Block *> emitted_subroutines;

	const Block *active_block = nullptr;

	// TODO: prelude section for types, etc.
	std::string result; // TODO: refactor to 'code'
	std::string indent = "    ";

	// TODO: indentation as well
	
	GLSL(const SharedBlockReference &sbr);

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
		std::string impl(Argument arg);
		std::string impl(Local local, Reference ref);
		
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
		std::string impl(Argument arg);
		std::string impl(Invocation invocation);
		std::string impl(const BuiltinIntrinsic &builtin);
		std::string main(Reference expression);
		DEFINE_CALL_OPERATOR(std::string);
	} expression;

	struct generate_statement {
		GLSL &parent;

		void impl(Store store);
		void impl(Invocation invocation);
		void impl(Branch branch);
		void impl(Loop loop);
		void impl(Local local, Reference ref);

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

private:
	void set_active_block(const Block &blk);
	void reset_state();
	void collect_blocks(std::vector <const Block *> &blocks) const;
	void emit_preamble();
	void emit_aggregate_decls();
	void emit_thread_inputs();
	void emit_thread_outputs();
	void emit_global_resources();
	void collect_push_constant_indices(
		const std::vector <const Block *> &blocks,
		std::map <void *, uint32_t> &pc_indices
	);
	std::string resource_key(const GlobalResource &grsrc) const;
	void emit_resource_decl(GlobalResource &grsrc);
	void emit_subroutine_functions();
	void emit_subroutine_function(const Block &blk, const std::string &name);
	void emit_main_function();
	void emit_block_statements(const Block &blk);
	std::string subroutine_return_type(const Block &blk, uint32_t &out_argi);
};

} // namespace generators

#undef DEFINE_CALL_OPERATOR
