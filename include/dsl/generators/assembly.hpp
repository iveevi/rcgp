#pragma once

#include <map>

#include "../instructions.hpp"
#include "../../msv/static_string.hpp"
#include "../../util/logging.hpp"

namespace generators {

struct Assembly {
	const Block &block;

	std::map <std::intptr_t, uint32_t> ids;

	std::string stringify(Reference ref) {
		if (!ref)
			return "nil";

		auto addr = intptr_t(ref.get());

		auto it = ids.find(addr);
		if (it != ids.end()) {
			return fmt::format("${}", it->second);
		} else {
			auto id = ids.size();
			ids[addr] = id;
			return fmt::format("${}", id);
		}
	}

	#define $assign stringify(ref) + " = " +
	
	std::string stringify_impl(auto x, Reference ref) {
		// Implementation fallback
		fatal("stringify_impl not implemented for {}", $ss_type(decltype(x)).view());
	}

	std::string stringify(Constant x, Reference ref) {
		return $assign std::visit([](auto x) {
			return fmt::format("{}", x);
		}, x);
	}
	
	std::string stringify_impl(PrimitiveType x, Reference ref) {
		vswitch(x) {
		vcase(bool): return "bool";
		vcase(int32_t): return "i32";
		vcase(uint32_t): return "u32";
		vcase(float): return "f32";

		vcase(VectorType <float, 1>): return "vec1";
		vcase(VectorType <float, 2>): return "vec2";
		vcase(VectorType <float, 3>): return "vec3";
		vcase(VectorType <float, 4>): return "vec4";

		vcase(MatrixType <float, 4, 4>): return "mat4";
		default:
			break;
		}

		return "primitive(?)";
	}

	std::string stringify_impl(AggregateType x, Reference ref) {
		std::string result;
		for (size_t i = 0; i < x.size(); i++) {
			result += stringify(x[i]);
			if (i + 1 < x.size())
				result += ", ";
		}

		return "aggregate(" + result + ")";
	}
	
	std::string stringify(Type x, Reference ref) {
		return $assign std::visit([&](auto x) {
			return stringify_impl(x, ref);
		}, x);
	}

	std::string stringify(Operation x, Reference ref) {
		std::string op = "?";
		switch (x.code) {
		case Operation::eAdd: op = "add"; break;
		case Operation::eMultiply: op = "mul"; break;
		default:
			break;
		}

		return $assign fmt::format("{}({}, {})",
			op, stringify(x.a), stringify(x.b));
	}
	
	std::string stringify(Store x, Reference ref) {
		return fmt::format("store {} {}",
			stringify(x.destination), stringify(x.source));
	}

	std::string stringify(FieldAccess x, Reference ref) {
		// TODO: use hints for more info
		return $assign fmt::format("field {}:{}",
			stringify(x.value), x.fidx);
	}

	std::string stringify_impl(Argument x, Reference ref) {
		return fmt::format("argument {}:{}",
			stringify(x.type), x.argi);
	}

	std::string stringify_impl(GlobalResource x, Reference ref) {
		std::string kind = "?";
		switch (x.kind) {
		case GlobalResource::eXConstant: kind = "xconstant"; break;
		case GlobalResource::ePushConstant: kind = "push_constant"; break;
		case GlobalResource::eConstantBuffer: kind = "constantm_buffer"; break;
		case GlobalResource::eStorageBuffer: kind = "storage_buffer"; break;
		case GlobalResource::eSampler: kind = "sampler"; break;
		default:
			break;
		}

		auto opint = [](const std::optional <uint32_t> &val) {
			return val ? fmt::format("{}", val.value()) : "nil";
		};

		return fmt::format("{}({}, {}:{})", kind,
			stringify(x.type), opint(x.group), opint(x.index));
	}
	
	std::string stringify_impl(ThreadInput x, Reference ref) {
		return fmt::format("thread in({}, {})",
			stringify(x.type), x.argi);
	}

	std::string stringify(ThreadOutput::Properties properties) {
		switch (properties) {
		case ThreadOutput::eSmooth: return "smooth";
		case ThreadOutput::eFlat: return "flat";
		case ThreadOutput::eNoPerspective: return "noperspective";
		default:
			break;
		}

		return "?";
	}
	
	std::string stringify_impl(ThreadOutput x, Reference ref) {
		return fmt::format("thread out({}, {}, {})",
			stringify(x.type), x.argi, stringify(x.properties));
	}
	
	std::string stringify_impl(GlobalIntrinsic x, Reference ref) {
		switch (x) {
		case GlobalIntrinsic::eSVPosition: return "SVPosition";
		default:
			break;
		}
		
		return "?";
	}

	std::string stringify(Intrinsic x, Reference ref) {
		return $assign std::visit([&](auto x) {
			return stringify_impl(x, ref);
		}, x);
	}

	std::string stringify(Construct x, Reference ref) {
		std::string result = fmt::format("new {}(", stringify(x.type));
		for (size_t i = 0; i < x.args.size(); i++) {
			result += stringify(x.args[i]);
			if (i + 1 < x.args.size())
				result += ", ";
		}
		result += ")";

		return $assign result;
	}

	#undef $assign

	std::string stringify(auto x, Reference ref) {
		return "?";
	}

	std::string stringify(ExecutionModel model) {
		switch (model) {
		case ExecutionModel::eAgnostic: return "agnostic";
		case ExecutionModel::eVulkanVertex: return "Vulkan vertex shader";
		case ExecutionModel::eVulkanFragment: return "Vulkan fragment shader";
		case ExecutionModel::eVulkanCompute: return "Vulkan compute shader";
		default:
			break;
		}

		return "?";
	}

	std::string generate(size_t tabs = 0) {
		std::string result = "block {\n";

		result += "  context {\n";
		result += "    model: " + stringify(block.context.model) + ",\n";
		for (auto tin : block.context.thread_inputs) {
			result += fmt::format("    thread in {}: {},\n",
				tin.argi, stringify(tin.type));
		}

		for (auto tout : block.context.thread_outputs) {
			result += fmt::format("    thread out {}: {} ({}),\n",
				tout.argi, stringify(tout.type), stringify(tout.properties));
		}

		result += "  }\n";
		
		for (auto &instr : block) {
			auto str = std::visit([&](auto x) {
				return stringify(x, instr);
			}, *instr);
			auto loc = instr->debug_info.origin;
			auto rel = std::filesystem::relative(loc.file_name());
			result += fmt::format("  {:<40} ; from {}:{}\n",
				str, rel.string(), loc.line());
		}
		result += "}";

		return result;
	}
};

} // namespace generators
