#pragma once

#include "../instructions.hpp"


#define DEFINE_CALL_OPERATOR() auto operator()(Reference r) { return main(r); }

namespace generators {

// TODO: compiler-like logging...
struct GLSL {
	const Block &block;

	std::vector <std::string> thread_inputs;

	// TODO: prelude section for types, etc.
	std::string result; // TODO: refactor to 'code'

	// TODO: indentation as well
	
	GLSL(const Block &block_)
		: block(block_),
		reference(*this),
		expression(*this),
		statement(*this),
		type(*this)
	{
		// TODO: generate thread inputs and resources, etc
		// here itself...
	}
	
	struct generate_reference {
		GLSL &parent;

		std::string impl(auto x) {
			// TODO: these should be exceptions...
			return "?";
		}

		std::string impl(Intrinsic intrinsic) {
			vswitch (intrinsic) {
			vcase(GlobalIntrinsic):
			{
				switch (intrinsic.as <GlobalIntrinsic> ()) {
				case GlobalIntrinsic::eSVPosition:
					return "gl_Position";
				default:
					return "?";
				}
			}

			vcase(ThreadOutput):
			{
				auto tout = intrinsic.as <ThreadOutput> ();
				return fmt::format("lout{}", tout.argi);
			}

			default:
				return "?";
			}
		}
		
		std::string main(Reference reference) {
			auto ftn = [&](auto x) { return impl(x); };
			return std::visit(ftn, *reference);
		}
		
		DEFINE_CALL_OPERATOR();
	} reference;

	struct generate_expression {
		GLSL &parent;

		std::string impl(auto x) {
			return "?";
		}
		
		std::string impl(Constant value) {
			vswitch (value) {
			vcase(int): return fmt::format("{}", value.as <int32_t> ());
			vcase(float): return fmt::format("{}", value.as <float> ());
			default:
				return "?";
			}
		}

		std::string impl(Operation operation) {
			// TODO: handle unary minus and not

			std::string op = "?";
			switch (operation.code) {
			case Operation::eAdd: op = "+"; break;
			case Operation::eMultiply: op = "*"; break;
			default:
				break;
			}

			return fmt::format("({} {} {})",
				main(operation.a), op, main(operation.b));
		}
		
		std::string impl(Intrinsic intrinsic) {
			vswitch (intrinsic) {
			vcase(ThreadInput):
			{
				auto &ti = intrinsic.as <ThreadInput> ();
				return parent.thread_inputs[ti.argi];
			}

			default:
				return "?";
			}
		}
		
		std::string impl(Construct construct) {
			std::string out = parent.type.main(construct.type) + "(";

			auto nargs = construct.args.size();
			for (size_t i = 0; i < nargs; i++) {
				out += main(construct.args[i]);
				if (i + 1 < nargs)
					out += ", ";
			}

			return out + ")";
		}

		std::string main(Reference expression) {
			auto ftn = [&](auto x) { return impl(x); };
			return std::visit(ftn, *expression);
		}

		DEFINE_CALL_OPERATOR();
	} expression;

	struct generate_statement {
		GLSL &parent;

		void impl(Store store) {
			// TODO: assign() method
			parent.result += fmt::format("\t{} = {};\n",
				parent.reference(store.destination),
				parent.expression(store.source));
		}

		void impl(auto x) {
			parent.result += "\t?\n";
		}

		void main(Reference instruction) {
			auto ftn = [&](auto x) { return impl(x); };
			return std::visit(ftn, *instruction);
		}
		
		DEFINE_CALL_OPERATOR();
	} statement;

	struct generate_type {
		GLSL &parent;

		// TODO: options... (e.g. version)
		std::string impl(PrimitiveType type) {
			// TODO: use specializations for this...
			vswitch (type) {
			vcase(int): return "int";
			vcase(VectorType <float, 2>): return "vec2";
			vcase(VectorType <float, 3>): return "vec3";
			vcase(VectorType <float, 4>): return "vec4";
			vcase(MatrixType <float, 4, 4>): return "mat4";
			default:
				break;
			}

			return "?";
		}

		std::string impl(Type type) {
			auto ftn = [&](auto x) { return impl(x); };
			return std::visit(ftn, type);
		}
		
		std::string impl(auto type) {
			__builtin_trap();
		}
		
		std::string main(Reference type) {
			auto ftn = [&](auto x) { return impl(x); };
			return std::visit(ftn, *type);
		}
		
		DEFINE_CALL_OPERATOR();
	} type;

	std::string generate(size_t tabs = 0) {
		if (block.context.model != ExecutionModel::eAgnostic) {
			result = "#version 460\n";

			// TODO: generate type definitions

			// TODO: generate globals

			// TODO: model specific thread inputs...
			result += "\n";

			thread_inputs.reserve(block.context.thread_inputs.size());
			for (auto &tin : block.context.thread_inputs) {
				thread_inputs.push_back(fmt::format("lin{}", tin.argi));
				result += fmt::format("layout (location = {}) in {} lin{};\n",
			  		tin.argi, type.main(tin.type), tin.argi);
			}

			result += "\n";
			
			for (auto &tout : block.context.thread_outputs) {
				std::string qualifier = "?";
				switch (tout.properties) {
				case ThreadOutput::eSmooth: qualifier = "smooth"; break;
				case ThreadOutput::eFlat: qualifier = "flat"; break;
				case ThreadOutput::eNoPerspective: qualifier = "noperspective"; break;
				default:
					break;
				}

				result += fmt::format("layout (location = {}) {} out {} lout{};\n",
			  		tout.argi, qualifier, type.main(tout.type), tout.argi);
			}

			// TODO: generate functions
			result += "\n";

			result += "void main()\n";
			result += "{\n";

			// gather instructions to manifest
			// TODO: split policy...
			std::vector <Reference> manifest;

			for (auto &instr : block) {
				vswitch ((*instr)) {
				vcase(Store):
					manifest.push_back(instr);
					break;
				default:
					break;
				}
			}

			fmt::println("instructions to manifest: {}", manifest.size());
			for (auto &instr : manifest)
				statement(instr);

			result += "}";

			return result;
		} else {
			__builtin_trap();
		}
	}
};

} // namespace generators
