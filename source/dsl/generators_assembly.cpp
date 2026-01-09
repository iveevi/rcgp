#include <filesystem>
#include <set>
#include <vector>

#include <fmt/format.h>

#include "dsl/generators.hpp"
#include "util/logging.hpp"

namespace {

struct AssemblyContext {
	const SharedBlockReference &sbr;
	std::map <intptr_t, uint32_t> ids;
};

std::string stringify(AssemblyContext &ctx, Reference ref);
std::string stringify(AssemblyContext &ctx, Constant x, Reference ref);
std::string stringify_type(AssemblyContext &ctx, PrimitiveType x, Reference ref);
std::string stringify_type(AssemblyContext &ctx, AggregateType x, Reference ref);
std::string stringify_type(AssemblyContext &ctx, ArrayType x, Reference ref);
std::string stringify(AssemblyContext &ctx, Type x, Reference ref);
std::string stringify(AssemblyContext &ctx, Operation x, Reference ref);
std::string stringify(AssemblyContext &ctx, Store x, Reference ref);
std::string stringify(AssemblyContext &ctx, ArrayAccess x, Reference ref);
std::string stringify(AssemblyContext &ctx, FieldAccess x, Reference ref);
std::string stringify(AssemblyContext &ctx, Argument x, Reference ref);
std::string stringify(GlobalResourceLayout layout);
std::string stringify(AssemblyContext &ctx, GlobalResource x, Reference ref);
std::string stringify(AssemblyContext &ctx, ThreadInput x, Reference ref);
std::string stringify_rate_properties(RateProperties properties);
std::string stringify(AssemblyContext &ctx, ThreadOutput x, Reference ref);
std::string stringify(AssemblyContext &ctx, Invocation x, Reference ref);
std::string stringify(AssemblyContext &ctx, GlobalIntrinsic x, Reference ref);
std::string stringify(AssemblyContext &ctx, Construct x, Reference ref);
std::string stringify(AssemblyContext &ctx, BuiltinIntrinsic x, Reference ref);
std::string stringify(AssemblyContext &ctx, Swizzle x, Reference ref);
std::string stringify(AssemblyContext &ctx, Branch x, Reference ref);
std::string stringify(AssemblyContext &ctx, Loop x, Reference ref);
std::string stringify(AssemblyContext &ctx, Local x, Reference ref);
std::string stringify(AssemblyContext &ctx, Block x, Reference ref);
std::string stringify(ShaderStage stage);
std::string generate(AssemblyContext &ctx, size_t tabs = 0, bool emit_branches = true);
std::string stringify_block_ref(AssemblyContext &ctx, const SharedBlockReference &blk);
std::string generate_block_body(AssemblyContext &ctx, const SharedBlockReference &blk, const std::string &indent);
void emit_debug_line(std::string &result, const std::string &text, const std::source_location &origin);
void emit_branch_block(AssemblyContext &ctx, const Branch &branch, Reference instr,
	std::string &result);
void emit_loop_block(AssemblyContext &ctx, const Loop &loop, Reference instr,
	std::string &result);

std::string stringify(AssemblyContext &ctx, Reference ref)
{
	if (!ref)
		return "nil";

	auto addr = intptr_t(ref.get());

	auto it = ctx.ids.find(addr);
	if (it != ctx.ids.end()) {
		return fmt::format("${}", it->second);
	} else {
		auto id = ctx.ids.size();
		ctx.ids[addr] = id;
		return fmt::format("${}", id);
	}
}

#define $assign stringify(ctx, ref) + " = " +

std::string stringify(AssemblyContext &ctx, Constant x, Reference ref)
{
	return $assign std::visit([](auto x) {
		return fmt::format("{}", x);
	}, x);
}

std::string stringify_type(AssemblyContext &ctx, PrimitiveType x, Reference ref)
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

	vcase(MatrixType <int32_t, 2, 2>): return "mat2i";
	vcase(MatrixType <int32_t, 3, 3>): return "mat3i";
	vcase(MatrixType <int32_t, 4, 4>): return "mat4i";
	vcase(MatrixType <uint32_t, 2, 2>): return "mat2u";
	vcase(MatrixType <uint32_t, 3, 3>): return "mat3u";
	vcase(MatrixType <uint32_t, 4, 4>): return "mat4u";
	vcase(MatrixType <float, 2, 2>): return "mat2";
	vcase(MatrixType <float, 3, 3>): return "mat3";
	vcase(MatrixType <float, 4, 4>): return "mat4";
	default:
		break;
	}

	return "primitive(?)";
}

std::string stringify_type(AssemblyContext &ctx, AggregateType x, Reference ref)
{
	std::string result;
	for (size_t i = 0; i < x.size(); i++) {
		result += stringify(ctx, x[i]);
		if (i + 1 < x.size())
			result += ", ";
	}

	return "aggregate(" + result + ")";
}

std::string stringify_type(AssemblyContext &ctx, ArrayType x, Reference ref)
{
	return fmt::format("array({}, {})",
		stringify(ctx, x.base), x.size);
}

std::string stringify(AssemblyContext &ctx, Type x, Reference ref)
{
	return $assign std::visit([&](auto x) {
		return stringify_type(ctx, x, ref);
	}, x);
}

std::string stringify(AssemblyContext &ctx, Operation x, Reference ref)
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
		op, stringify(ctx, x.a), stringify(ctx, x.b));
}

std::string stringify(AssemblyContext &ctx, Store x, Reference ref)
{
	return fmt::format("store {} {}",
		stringify(ctx, x.destination), stringify(ctx, x.source));
}

std::string stringify(AssemblyContext &ctx, ArrayAccess x, Reference ref)
{
	return $assign fmt::format("index({}, {})",
		stringify(ctx, x.value), stringify(ctx, x.index));
}

std::string stringify(AssemblyContext &ctx, FieldAccess x, Reference ref)
{
	return $assign fmt::format("field {}:{}",
		stringify(ctx, x.value), x.fidx);
}

std::string stringify(AssemblyContext &ctx, Argument x, Reference ref)
{
	return $assign fmt::format("argument {}:{}",
		stringify(ctx, x.type), x.argi);
}

std::string stringify(GlobalResourceLayout layout)
{
	switch (layout) {
	case GlobalResourceLayout::eScalar: return "scalar";
	case GlobalResourceLayout::eStd430: return "std430";
	case GlobalResourceLayout::eNone: return "-";
	default:
		return "?";
	}
}

std::string stringify(AssemblyContext &ctx, GlobalResource x, Reference ref)
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
		stringify(ctx, x.type), opint(x.group), opint(x.index), stringify(x.layout));
}

std::string stringify(AssemblyContext &ctx, ThreadInput x, Reference ref)
{
	return $assign fmt::format("thread in({}, {})",
		stringify(ctx, x.type), x.argi);
}

std::string stringify_rate_properties(RateProperties properties)
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

std::string stringify(AssemblyContext &ctx, ThreadOutput x, Reference ref)
{
	return $assign fmt::format("thread out({}, {}, {})",
		stringify(ctx, x.type), x.argi,
		stringify_rate_properties(x.properties));
}

std::string stringify(AssemblyContext &ctx, GlobalIntrinsic x, Reference ref)
{
	switch (x) {
	case GlobalIntrinsic::eScreenPosition:
		return $assign "SVPosition";
	default:
		break;
	}

	return "?";
}

std::string stringify(AssemblyContext &ctx, Construct x, Reference ref)
{
	std::string result = fmt::format("new {}(", stringify(ctx, x.type));

	// TODO: method for args:
	for (size_t i = 0; i < x.args.size(); i++) {
		result += stringify(ctx, x.args[i]);
		if (i + 1 < x.args.size())
			result += ", ";
	}
	result += ")";

	return $assign result;
}

std::string stringify(AssemblyContext &ctx, BuiltinIntrinsic x, Reference ref)
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
	case BuiltinIntrinsicCode::ePow: ftn = "pow"; break;
	case BuiltinIntrinsicCode::eMin: ftn = "min"; break;
	default:
		break;
	}

	std::string args;
	for (size_t i = 0; i < x.args.size(); i++) {
		args += stringify(ctx, x.args[i]);
		if (i + 1 < x.args.size())
			args += ", ";
	}

	return $assign ftn + "(" + args + ")";
}

std::string stringify(AssemblyContext &ctx, Swizzle x, Reference ref)
{
	return $assign fmt::format("swizzle({}, {})",
		stringify(ctx, x.value), swizzle_string(x.code));
}

std::string stringify(AssemblyContext &ctx, Branch x, Reference ref)
{
	std::string result = "branch(";
	for (size_t i = 0; i < x.segments.size(); i++) {
		auto &segment = x.segments[i];
		result += fmt::format("{}: {}", stringify(ctx, segment.cond),
			stringify_block_ref(ctx, segment.body));
		if (i + 1 < x.segments.size())
			result += ", ";
	}

	if (x.fallback.has_value()) {
		if (!x.segments.empty())
			result += ", ";
		result += fmt::format("else: {}", stringify_block_ref(ctx, x.fallback.value()));
	}
	result += ")";

	return $assign result;
}

std::string stringify(AssemblyContext &ctx, Loop x, Reference ref)
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
		result += fmt::format(", init: {}", stringify_block_ref(ctx, x.init.value()));

	result += fmt::format(", cond: {}", stringify_block_ref(ctx, x.cond));

	if (x.step.has_value())
		result += fmt::format(", step: {}", stringify_block_ref(ctx, x.step.value()));

	result += fmt::format(", body: {}", stringify_block_ref(ctx, x.body));
	result += ")";

	return $assign result;
}

std::string stringify(AssemblyContext &ctx, Local x, Reference ref)
{
	return $assign fmt::format("local {}", stringify(ctx, x.type));
}

std::string stringify(AssemblyContext &ctx, Invocation x, Reference ref)
{
	std::string result = fmt::format("@{}(", (void *) x.sbr.get());
	for (size_t i = 0; i < x.args.size(); i++) {
		result += stringify(ctx, x.args[i]);
		if (i + 1 < x.args.size())
			result += ", ";
	}
	result += ")";

	return $assign result;
}

#undef $assign

std::string stringify(AssemblyContext &ctx, Block x, Reference ref)
{
	fatal("cannot generate assembly for block");
}

std::string stringify(ShaderStage stage)
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

void emit_debug_line(std::string &result, const std::string &text, const std::source_location &origin)
{
	auto rel = std::filesystem::relative(origin.file_name());
	result += fmt::format("  {:<40} ; from {}:{}\n",
		text, rel.string(), origin.line());
}

void emit_branch_block(AssemblyContext &ctx, const Branch &branch, Reference instr,
	std::string &result)
{
	for (auto &segment : branch.segments) {
		auto ref = stringify_block_ref(ctx, segment.body);
		result += fmt::format("  {} = {}\n",
			ref, generate_block_body(ctx, segment.body, "  "));
	}
	if (branch.fallback.has_value()) {
		auto ref = stringify_block_ref(ctx, branch.fallback.value());
		result += fmt::format("  {} = {}\n",
			ref, generate_block_body(ctx, branch.fallback.value(), "  "));
	}

	std::string line = fmt::format("{} = branch(", stringify(ctx, instr));
	emit_debug_line(result, line, instr->debug_info.origin);
	std::vector <std::string> entries;
	entries.reserve(branch.segments.size() + (branch.fallback ? 1 : 0));
	for (auto &segment : branch.segments) {
		entries.push_back(fmt::format("    {}: {}",
			stringify(ctx, segment.cond), stringify_block_ref(ctx, segment.body)));
	}
	if (branch.fallback.has_value()) {
		entries.push_back(fmt::format("    else: {}",
			stringify_block_ref(ctx, branch.fallback.value())));
	}
	for (size_t i = 0; i < entries.size(); i++) {
		result += entries[i];
		result += (i + 1 < entries.size()) ? ",\n" : "\n";
	}
	result += "  )\n";
}

void emit_loop_block(AssemblyContext &ctx, const Loop &loop, Reference instr,
	std::string &result)
{
	if (loop.init.has_value()) {
		auto ref = stringify_block_ref(ctx, loop.init.value());
		result += fmt::format("  {} = {}\n",
			ref, generate_block_body(ctx, loop.init.value(), "  "));
	}

	auto cond_ref = stringify_block_ref(ctx, loop.cond);
	result += fmt::format("  {} = {}\n",
		cond_ref, generate_block_body(ctx, loop.cond, "  "));

	if (loop.step.has_value()) {
		auto ref = stringify_block_ref(ctx, loop.step.value());
		result += fmt::format("  {} = {}\n",
			ref, generate_block_body(ctx, loop.step.value(), "  "));
	}

	auto body_ref = stringify_block_ref(ctx, loop.body);
	result += fmt::format("  {} = {}\n",
		body_ref, generate_block_body(ctx, loop.body, "  "));

	auto line = fmt::format("{} = loop(", stringify(ctx, instr));
	emit_debug_line(result, line, instr->debug_info.origin);
	result += fmt::format("    kind: {}\n",
		(loop.kind == LoopKind::eWhile) ? "while" : "for");
	if (loop.init.has_value())
		result += fmt::format("    init: {},\n", stringify_block_ref(ctx, loop.init.value()));
	result += fmt::format("    cond: {},\n", stringify_block_ref(ctx, loop.cond));
	if (loop.step.has_value())
		result += fmt::format("    step: {},\n", stringify_block_ref(ctx, loop.step.value()));
	result += fmt::format("    body: {}\n", stringify_block_ref(ctx, loop.body));
	result += "  )\n";
}

std::string generate(AssemblyContext &ctx, size_t tabs, bool emit_branches)
{
	std::string result = "block {\n";

	result += "  context {\n";
	result += fmt::format("    blkid: {},\n", (void *) ctx.sbr.get());
	result += "    model: " + stringify(ctx.sbr->context.model) + ",\n";

	for (auto arg : ctx.sbr->context.arguments) {
		result += fmt::format("    argument {}: {},\n",
			arg.argi, stringify(ctx, arg.type));
	}

	for (auto tin : ctx.sbr->context.thread_inputs) {
		result += fmt::format("    thread in {}: {},\n",
			tin.argi, stringify(ctx, tin.type));
	}

	for (auto tout : ctx.sbr->context.thread_outputs) {
		result += fmt::format("    thread out {}: {} ({}),\n",
			tout.argi, stringify(ctx, tout.type),
			stringify_rate_properties(tout.properties));
	}

	for (auto [k, v] : ctx.sbr->context.global_resources) {
		std::string set;

		size_t count = 0;
		for (auto &vv : v) {
			set += stringify(ctx, vv);
			if (++count < v.size())
				set += ", ";
		}

		result += fmt::format("    resource {}: {{{}}},\n", k, set);
	}

	result += "  }\n";

	for (auto &instr : *ctx.sbr) {
		if (emit_branches && instr->is <Branch> ()) {
			emit_branch_block(ctx,
				instr->as <Branch> (),
				instr, result
			);
			continue;
		}

		if (emit_branches && instr->is <Loop> ()) {
			emit_loop_block(ctx,
				instr->as <Loop> (),
				instr, result
			);
			continue;
		}

		auto str = std::visit([&](auto x) {
			return stringify(ctx, x, instr);
		}, *instr);
		emit_debug_line(result, str, instr->debug_info.origin);
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

	scan_block(ctx.sbr);
	for (size_t i = 0; i < blocks.size(); i++)
		scan_block(blocks[i]);

	return result;
}

std::string stringify_block_ref(AssemblyContext &ctx, const SharedBlockReference &blk)
{
	if (!blk)
		return "nil";

	auto addr = intptr_t(blk.get());
	auto it = ctx.ids.find(addr);
	if (it != ctx.ids.end())
		return fmt::format("${}", it->second);

	auto id = ctx.ids.size();
	ctx.ids[addr] = id;
	return fmt::format("${}", id);
}

std::string generate_block_body(AssemblyContext &ctx, const SharedBlockReference &blk, const std::string &indent)
{
	std::string result = "block {\n";

	auto prefix = indent + "  ";
	auto pad_width = 40;
	if (indent.size() < static_cast <size_t> (pad_width))
		pad_width -= static_cast <int> (indent.size());

	if (blk->empty())
		result += prefix + "(empty block)\n";

	for (auto &instr : *blk) {
		auto str = std::visit([&](auto x) {
			return stringify(ctx, x, instr);
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

} // namespace

std::string generate_assembly(const SharedBlockReference &sbr, size_t tabs)
{
	AssemblyContext ctx { sbr, {} };
	return generate(ctx, tabs);
}
