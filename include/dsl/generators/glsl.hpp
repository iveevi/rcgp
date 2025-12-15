#pragma once

#include "../../meta/static_string.hpp"
#include "../../util/logging.hpp"
#include "../instructions.hpp"

#define DEFINE_CALL_OPERATOR() auto operator()(Reference r) { return main(r); }

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

	std::string layout_string(GlobalResource::Layout layout) const {
		switch (layout) {
		case GlobalResource::Layout::eScalar: return "scalar, ";
		case GlobalResource::Layout::eStd430: return "std430, ";
		case GlobalResource::Layout::eUnknown:
		default:
			return "";
		}
	}
	
	struct generate_reference {
		GLSL &parent;

		std::string impl(auto x) {
			fatal("generate_reference::impl not implemented for {}",
	 			$ss_type(decltype(x)).view());
		}

		std::string impl(GlobalIntrinsic gi) {
			switch (gi) {
			case GlobalIntrinsic::eScreenPosition: return "gl_Position";
			case GlobalIntrinsic::eInstanceIndex: return "gl_InstanceIndex";
			default:
				return "?";
			}
		}

		std::string impl(GlobalResource grsrc) {
			// TODO: different if its a push constant
			return fmt::format("r{}_i{}",
		      		grsrc.group.value_or(-1),
		      		grsrc.index.value_or(-1));
		}

		std::string impl(ThreadOutput tout) {
			return fmt::format("lout{}", tout.argi);
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
			fatal("generate_expression::impl not implemented for {}",
	 			$ss_type(decltype(x)).view());
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

		std::string impl(ThreadInput tin) {
			return parent.thread_inputs[tin.argi];
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

		std::string impl(FieldAccess access) {
			return fmt::format("{}.f{}", main(access.value), access.fidx);
		}

		std::string impl(ArrayAccess access) {
			return fmt::format("{}[{}]", main(access.value), main(access.index));
		}

		std::string impl(GlobalResource grsrc) {
			return parent.reference.impl(grsrc);
		}
		
		std::string impl(GlobalIntrinsic intrinsic) {
			return parent.reference.impl(intrinsic);
		}

		std::string impl(const BuiltinIntrinsic &builtin) {
			std::string out = "?";

			switch (builtin.code) {
			case BuiltinIntrinsic::eDot: out = "dot"; break;
			case BuiltinIntrinsic::eMax: out = "max"; break;
			case BuiltinIntrinsic::eNormalize: out = "normalize"; break;
			case BuiltinIntrinsic::eTranspose: out = "transpose"; break;
			case BuiltinIntrinsic::eInverse: out = "inverse"; break;
			default:
				break;
			}

			out += "(";

			auto nargs = builtin.args.size();
			for (size_t i = 0; i < nargs; i++) {
				out += main(builtin.args[i]);
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
			parent.result += fmt::format("    {} = {};\n",
				parent.reference(store.destination),
				parent.expression(store.source));
		}

		void impl(auto x) {
			parent.result += "    ?\n";
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
		std::string impl(const PrimitiveType &primitive) {
			// TODO: use specializations for this...
			vswitch (primitive) {
			vcase(int): return "int";
			vcase(float): return "float";
			vcase(VectorType <float, 2>): return "vec2";
			vcase(VectorType <float, 3>): return "vec3";
			vcase(VectorType <float, 4>): return "vec4";
			vcase(MatrixType <float, 3, 3>): return "mat3";
			vcase(MatrixType <float, 4, 4>): return "mat4";
			default:
				break;
			}

			return "?";
		}

		std::string impl(const Type &type) {
			return std::visit([&](const auto &x) { return impl(x); }, type);
		}

		// TODO: post and pre?
		std::string impl(const ArrayType &array) {
			auto size = (array.size > 0)
				? fmt::format("[{}]", array.size)
				: "[]";

			return main(array.base) + size;
		}

		std::string impl(const AggregateType &aggregate) {
			auto addr = &aggregate;
			// TODO: assert
			if (parent.aggregate_names.contains(addr))
				return parent.aggregate_names[addr];
			return "?";
		}
		
		std::string impl(auto type) {
			fatal("generate_type::impl not implemented for {}",
	 			$ss_type(decltype(type)).view());
		}
		
		std::string main(Reference type) {
			return std::visit([&](const auto &x) { return impl(x); }, *type);
		}
		
		DEFINE_CALL_OPERATOR();
	} type;

	std::string generate(size_t tabs = 0) {
		if (block.context.model != ExecutionModel::eAgnostic) {
			result = "#version 460\n";

			result += "#extension GL_EXT_scalar_block_layout : require\n\n";

			result += "\n";

			// TODO: generate type definitions
			// TODO: avoid duplicate definitions by
			// associating aggregates to a type index if possible
			// OR in reconstruct_type search the buffer for any existing
			// generations and return that instead... aggregate_cache <T>
			// needs a type_index <T> mechanism

			size_t aggregate_counter = 0;
			for (auto &instr : block) {
				if (not instr->is <Type> ())
					continue;

				auto &type = instr->as <Type> ();
				if (not type.is <AggregateType> ())
					continue;

				auto &agg = type.as <AggregateType> ();

				result += fmt::format("struct AGG{} {{\n", aggregate_counter++);

				size_t field_counter = 0;
				for (auto &field : agg) {
					result += fmt::format("    {} f{};\n",
			   			this->type.main(field), field_counter++);
				}

				result += "};\n\n";

				// TODO: name hints and all...
				aggregate_names.emplace(&agg, fmt::format("AGG{}", aggregate_counter - 1));
			}

			// TODO: generate globals

			thread_inputs.reserve(block.context.thread_inputs.size());
			for (auto &tin : block.context.thread_inputs) {
				thread_inputs.push_back(fmt::format("lin{}", tin.argi));
				result += fmt::format("layout (location = {}) in {} lin{};\n",
			  		tin.argi, type.main(tin.type), tin.argi);
			}

			if (block.context.thread_inputs.size())
				result += "\n";
			
			for (auto &tout : block.context.thread_outputs) {
				std::string qualifier = "? ";
				switch (tout.properties) {
				case RateProperties::eFlat: qualifier = "flat "; break;
				case RateProperties::eNone: qualifier = ""; break;
				case RateProperties::eNoPerspective: qualifier = "noperspective "; break;
				case RateProperties::eSmooth: qualifier = "smooth "; break;
				default:
					break;
				}

				result += fmt::format("layout (location = {}) {}out {} lout{};\n",
			  		tout.argi, qualifier, type.main(tout.type), tout.argi);
			}

			if (block.context.thread_outputs.size())
				result += "\n";

			for (auto &[_, refs] : block.context.global_resources) {
				for (auto &ref : refs) {
					auto &grsrc = ref->as <GlobalResource> ();
					if (grsrc.kind == GlobalResource::eSampler) {
						fatal("not implemented");
					} else {
						std::string modifier;
						switch (grsrc.kind) {
						case GlobalResource::eUniformBuffer: modifier = "uniform"; break;
						case GlobalResource::eStorageBuffer: modifier = "buffer"; break;
						case GlobalResource::ePushConstant:
						default:
							fatal("unsupported global resource kind");
						}

						auto group = grsrc.group.value_or(-1);
						auto index = grsrc.index.value_or(-1);
						result += fmt::format("layout ({}set = {}, binding = {}) {} R{}{} {{\n",
							layout_string(grsrc.layout), group, index, modifier, group, index);
						result += fmt::format("    {} r{}_i{};\n", type.main(grsrc.type), group, index);
						result += "};\n\n";
					}
				}
			}

			// TODO: generate functions

			result += "void main()\n";
			result += "{\n";

			// gather instructions to manifest
			// TODO: splitting policy...
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

			for (auto &instr : manifest)
				statement(instr);

			result += "}";

			return result;
		} else {
			fatal("per-function generation is not supported");
		}
	}
};

} // namespace generators
