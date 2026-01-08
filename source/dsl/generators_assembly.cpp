#include <filesystem>
#include <set>
#include <vector>

#include <fmt/format.h>

#include "util/logging.hpp"
#include "dsl/generators_assembly.hpp"

namespace generators {

std::string Assembly::stringify(Reference ref)
{
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

std::string Assembly::stringify(Constant x, Reference ref)
{
	return $assign std::visit([](auto x) {
		return fmt::format("{}", x);
	}, x);
}

std::string Assembly::stringify_type(PrimitiveType x, Reference ref)
{
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

std::string Assembly::stringify_type(AggregateType x, Reference ref)
{
	std::string result;
	for (size_t i = 0; i < x.size(); i++) {
		result += stringify(x[i]);
		if (i + 1 < x.size())
			result += ", ";
	}

	return "aggregate(" + result + ")";
}

std::string Assembly::stringify_type(ArrayType x, Reference ref)
{
	return fmt::format("array({}, {})",
		stringify(x.base), x.size);
}

std::string Assembly::stringify(Type x, Reference ref)
{
	return $assign std::visit([&](auto x) {
		return stringify_type(x, ref);
	}, x);
}

std::string Assembly::stringify(Operation x, Reference ref)
{
	std::string op = "?";
	switch (x.code) {
	case OperationCode::eAdd: op = "add"; break;
	case OperationCode::eSubtract: op = "sub"; break;
	case OperationCode::eMultiply: op = "mul"; break;
	case OperationCode::eDivide: op = "div"; break;
	case OperationCode::eEqual: op = "eq"; break;
	case OperationCode::eNotEqual: op = "neq"; break;
	case OperationCode::eLess: op = "lt"; break;
	case OperationCode::eLessEqual: op = "lte"; break;
	case OperationCode::eGreater: op = "gt"; break;
	case OperationCode::eGreaterEqual: op = "gte"; break;
	default:
		break;
	}

	return $assign fmt::format("{}({}, {})",
		op, stringify(x.a), stringify(x.b));
}

std::string Assembly::stringify(Store x, Reference ref)
{
	return fmt::format("store {} {}",
		stringify(x.destination), stringify(x.source));
}

std::string Assembly::stringify(ArrayAccess x, Reference ref)
{
	return $assign fmt::format("index({}, {})",
		stringify(x.value), stringify(x.index));
}

std::string Assembly::stringify(FieldAccess x, Reference ref)
{
	return $assign fmt::format("field {}:{}",
		stringify(x.value), x.fidx);
}

std::string Assembly::stringify(Argument x, Reference ref)
{
	return $assign fmt::format("argument {}:{}",
		stringify(x.type), x.argi);
}

std::string Assembly::stringify(GlobalResourceLayout layout)
{
	switch (layout) {
	case GlobalResourceLayout::eScalar: return "scalar";
	case GlobalResourceLayout::eStd430: return "std430";
	case GlobalResourceLayout::eNone: return "-";
	default:
		return "?";
	}
}

std::string Assembly::stringify(GlobalResource x, Reference ref)
{
	std::string kind = "?";
	switch (x.kind) {
	case GlobalResourceKind::ePushConstant: kind = "push_constant"; break;
	case GlobalResourceKind::eUniformBuffer: kind = "constant_buffer"; break;
	case GlobalResourceKind::eStorageBuffer: kind = "storage_buffer"; break;
	case GlobalResourceKind::eSampler: kind = "sampler"; break;
	default:
		break;
	}

	auto opint = [](const std::optional <uint32_t> &val) {
		return val ? fmt::format("{}", val.value()) : "nil";
	};

	return $assign fmt::format("{}({}, {}:{}, {})", kind,
		stringify(x.type), opint(x.group), opint(x.index), stringify(x.layout));
}

std::string Assembly::stringify(ThreadInput x, Reference ref)
{
	return $assign fmt::format("thread in({}, {})",
		stringify(x.type), x.argi);
}

std::string Assembly::stringify_rate_properties(RateProperties properties)
{
	switch (properties) {
	case RateProperties::eNone: return "-";
	case RateProperties::eSmooth: return "smooth";
	case RateProperties::eFlat: return "flat";
	case RateProperties::eNoPerspective: return "noperspective";
	default:
		break;
	}

	return "?";
}

std::string Assembly::stringify(ThreadOutput x, Reference ref)
{
	return $assign fmt::format("thread out({}, {}, {})",
		stringify(x.type), x.argi,
		stringify_rate_properties(x.properties));
}

std::string Assembly::stringify(GlobalIntrinsic x, Reference ref)
{
	switch (x) {
	case GlobalIntrinsic::eScreenPosition:
		return $assign "SVPosition";
	default:
		break;
	}

	return "?";
}

std::string Assembly::stringify(Construct x, Reference ref)
{
	std::string result = fmt::format("new {}(", stringify(x.type));

	// TODO: method for args:
	for (size_t i = 0; i < x.args.size(); i++) {
		result += stringify(x.args[i]);
		if (i + 1 < x.args.size())
			result += ", ";
	}
	result += ")";

	return $assign result;
}

std::string Assembly::stringify(BuiltinIntrinsic x, Reference ref)
{
	std::string ftn = "?";
	switch (x.code) {
	case BuiltinIntrinsicCode::eCross: ftn = "cross"; break;
	case BuiltinIntrinsicCode::eDFdx: ftn = "dFdx"; break;
	case BuiltinIntrinsicCode::eDFdy: ftn = "dFdy"; break;
	case BuiltinIntrinsicCode::eDFdxFine: ftn = "dFdxFine"; break;
	case BuiltinIntrinsicCode::eDFdyFine: ftn = "dFdyFine"; break;
	case BuiltinIntrinsicCode::eSample: ftn = "sample"; break;
	case BuiltinIntrinsicCode::eDot: ftn = "dot"; break;
	case BuiltinIntrinsicCode::eNormalize: ftn = "normalize"; break;
	case BuiltinIntrinsicCode::eMax: ftn = "max"; break;
	default:
		break;
	}

	std::string args;
	for (size_t i = 0; i < x.args.size(); i++) {
		args += stringify(x.args[i]);
		if (i + 1 < x.args.size())
			args += ", ";
	}

	return $assign ftn + "(" + args + ")";
}

std::string Assembly::stringify(Swizzle x, Reference ref)
{
	return $assign fmt::format("swizzle({}, {})",
		stringify(x.value), swizzle_string(x.code));
}

std::string Assembly::stringify(Branch x, Reference ref)
{
	std::string result = "branch(";
	for (size_t i = 0; i < x.segments.size(); i++) {
		auto &segment = x.segments[i];
		result += fmt::format("{}: {}", stringify(segment.cond),
			stringify_block_ref(segment.body));
		if (i + 1 < x.segments.size())
			result += ", ";
	}

	if (x.fallback.has_value()) {
		if (!x.segments.empty())
			result += ", ";
		result += fmt::format("else: {}", stringify_block_ref(x.fallback.value()));
	}
	result += ")";

	return $assign result;
}

std::string Assembly::stringify(Loop x, Reference ref)
{
	std::string result = "loop(";
	switch (x.kind) {
	case LoopKind::eWhile: result += "while"; break;
	case LoopKind::eFor: result += "for"; break;
	default:
		result += "?";
		break;
	}

	if (x.init.has_value())
		result += fmt::format(", init: {}", stringify_block_ref(x.init.value()));

	result += fmt::format(", cond: {}", stringify_block_ref(x.cond));

	if (x.step.has_value())
		result += fmt::format(", step: {}", stringify_block_ref(x.step.value()));

	result += fmt::format(", body: {}", stringify_block_ref(x.body));
	result += ")";

	return $assign result;
}

std::string Assembly::stringify(Local x, Reference ref)
{
	return $assign fmt::format("local {}", stringify(x.type));
}

std::string Assembly::stringify(Invocation x, Reference ref)
{
	std::string result = fmt::format("@{}(", (void *) x.sbr.get());
	for (size_t i = 0; i < x.args.size(); i++) {
		result += stringify(x.args[i]);
		if (i + 1 < x.args.size())
			result += ", ";
	}
	result += ")";

	return $assign result;
}

#undef $assign

std::string Assembly::stringify(Block x, Reference ref)
{
	fatal("cannot generate assembly for block");
}

std::string Assembly::stringify(ShaderStage stage)
{
	switch (stage) {
	case ShaderStage::eSubroutine: return "subroutine";
	case ShaderStage::eVertex: return "vertex shader";
	case ShaderStage::eFragment: return "fragment shader";
	case ShaderStage::eCompute: return "compute shader";
	default:
		break;
	}

	return "?";
}

std::string Assembly::generate(size_t tabs, bool emit_branches)
{
	std::string result = "block {\n";

	result += "  context {\n";
	result += fmt::format("    blkid: {},\n", (void *) sbr.get());
	result += "    model: " + stringify(sbr->context.model) + ",\n";

	for (auto arg : sbr->context.arguments) {
		result += fmt::format("    argument {}: {},\n",
			arg.argi, stringify(arg.type));
	}

	for (auto tin : sbr->context.thread_inputs) {
		result += fmt::format("    thread in {}: {},\n",
			tin.argi, stringify(tin.type));
	}

	for (auto tout : sbr->context.thread_outputs) {
		result += fmt::format("    thread out {}: {} ({}),\n",
			tout.argi, stringify(tout.type),
			stringify_rate_properties(tout.properties));
	}

	for (auto [k, v] : sbr->context.global_resources) {
		std::string set;

		size_t count = 0;
		for (auto &vv : v) {
			set += stringify(vv);
			if (++count < v.size())
				set += ", ";
		}

		result += fmt::format("    resource {}: {{{}}},\n", k, set);
	}

	result += "  }\n";

	// TODO: split this!
	for (auto &instr : *sbr) {
		auto loc = instr->debug_info.origin;
		auto rel = std::filesystem::relative(loc.file_name());
		auto emit_line = [&](const std::string &text) {
			result += fmt::format("  {:<40} ; from {}:{}\n",
				text, rel.string(), loc.line());
		};
		if (emit_branches && instr->is <Branch> ()) {
			auto &branch = instr->as <Branch> ();
			for (auto &segment : branch.segments) {
				auto ref = stringify_block_ref(segment.body);
				result += fmt::format("  {} = {}\n",
					ref, generate_block_body(segment.body, "  "));
			}
			if (branch.fallback.has_value()) {
				auto ref = stringify_block_ref(branch.fallback.value());
				result += fmt::format("  {} = {}\n",
					ref, generate_block_body(branch.fallback.value(), "  "));
			}

			std::string line = fmt::format("{} = branch(", stringify(instr));
			emit_line(line);
			std::vector <std::string> entries;
			entries.reserve(branch.segments.size() + (branch.fallback ? 1 : 0));
			for (auto &segment : branch.segments) {
				entries.push_back(fmt::format("    {}: {}",
					stringify(segment.cond), stringify_block_ref(segment.body)));
			}
			if (branch.fallback.has_value()) {
				entries.push_back(fmt::format("    else: {}",
					stringify_block_ref(branch.fallback.value())));
			}
			for (size_t i = 0; i < entries.size(); i++) {
				result += entries[i];
				result += (i + 1 < entries.size()) ? ",\n" : "\n";
			}
			result += "  )\n";
			continue;
		}
		if (emit_branches && instr->is <Loop> ()) {
			auto &loop = instr->as <Loop> ();
			if (loop.init.has_value()) {
				auto ref = stringify_block_ref(loop.init.value());
				result += fmt::format("  {} = {}\n",
					ref, generate_block_body(loop.init.value(), "  "));
			}

			auto cond_ref = stringify_block_ref(loop.cond);
			result += fmt::format("  {} = {}\n",
				cond_ref, generate_block_body(loop.cond, "  "));

			if (loop.step.has_value()) {
				auto ref = stringify_block_ref(loop.step.value());
				result += fmt::format("  {} = {}\n",
					ref, generate_block_body(loop.step.value(), "  "));
			}

			auto body_ref = stringify_block_ref(loop.body);
			result += fmt::format("  {} = {}\n",
				body_ref, generate_block_body(loop.body, "  "));

			auto line = fmt::format("{} = loop(", stringify(instr));
			emit_line(line);
			result += fmt::format("    kind: {}\n",
				(loop.kind == LoopKind::eWhile) ? "while" : "for");
			if (loop.init.has_value())
				result += fmt::format("    init: {},\n", stringify_block_ref(loop.init.value()));
			result += fmt::format("    cond: {},\n", stringify_block_ref(loop.cond));
			if (loop.step.has_value())
				result += fmt::format("    step: {},\n", stringify_block_ref(loop.step.value()));
			result += fmt::format("    body: {}\n", stringify_block_ref(loop.body));
			result += "  )\n";
			continue;
		}

		auto str = std::visit([&](auto x) {
			return stringify(x, instr);
		}, *instr);
		emit_line(str);
	}
	result += "}";

	if (!emit_branches)
		return result;

	std::vector <SharedBlockReference> blocks;
	std::set <const Block *> visited;

	auto enqueue = [&](const SharedBlockReference &blk) {
		if (!blk)
			return;
		if (visited.insert(blk.get()).second)
			blocks.push_back(blk);
	};

	auto scan_block = [&](const SharedBlockReference &blk) {
		for (auto &instr : *blk) {
			if (!instr->is <Branch> ())
				continue;
			auto &branch = instr->as <Branch> ();
			for (auto &segment : branch.segments)
				enqueue(segment.body);
			if (branch.fallback.has_value())
				enqueue(branch.fallback.value());
		}
	};

	scan_block(sbr);
	for (size_t i = 0; i < blocks.size(); i++)
		scan_block(blocks[i]);

	return result;
}

std::string Assembly::stringify_block_ref(const SharedBlockReference &blk)
{
	if (!blk)
		return "nil";

	auto addr = intptr_t(blk.get());
	auto it = ids.find(addr);
	if (it != ids.end())
		return fmt::format("${}", it->second);

	auto id = ids.size();
	ids[addr] = id;
	return fmt::format("${}", id);
}

std::string Assembly::generate_block_body(const SharedBlockReference &blk, const std::string &indent)
{
	std::string result = "block {\n";

	auto prefix = indent + "  ";
	auto pad_width = 40;
	if (indent.size() < static_cast <size_t> (pad_width))
		pad_width -= static_cast <int> (indent.size());

	for (auto &instr : *blk) {
		auto str = std::visit([&](auto x) {
			return stringify(x, instr);
		}, *instr);
		auto loc = instr->debug_info.origin;
		auto rel = std::filesystem::relative(loc.file_name());
		result += fmt::format("{}{} ; from {}:{}\n",
			prefix, fmt::format("{:<{}}", str, pad_width),
			rel.string(), loc.line());
	}

	result += indent + "}";
	return result;
}

} // namespace generators
