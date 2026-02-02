#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <print>
#include <set>
#include <source_location>
#include <vector>

#include <fmt/format.h>

#include "dsl/generators.hpp"
#include "dsl/instructions.hpp"

namespace rcgp {

struct AsmContext {
	const SharedBlockReference &sbr;
	std::map <intptr_t, uint32_t> ids;
	bool debug;

	std::string auxiliary(const std::string &text, const std::source_location &origin) const {
		if (debug) {
			auto rel = std::filesystem::relative(origin.file_name());
			return fmt::format("  {:<40} ; from {}:{}\n",
				text, rel.string(), origin.line());
		} else {
			return "  " + text + '\n';
		}
	}
};

std::string stringify(AsmContext &ctx, Reference ref)
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

std::string stringify_block_ref(AsmContext &ctx, const SharedBlockReference &blk)
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

#define $assign stringify(ctx, ref) + " = " +

std::string stringify(AsmContext &ctx, Constant x, Reference ref)
{
	return $assign std::visit([](auto x) {
		return fmt::format("{}", x);
	}, x);
}

std::string stringify_type(AsmContext &ctx, PrimitiveType x, Reference ref)
{
	vswitch(x) {
	vcase(bool): return "bool";
	vcase(int32_t): return "i32";
	vcase(uint32_t): return "u32";
	vcase(float): return "f32";

	vcase(VectorType <int32_t, 2>): return "int2";
	vcase(VectorType <int32_t, 3>): return "int3";
	vcase(VectorType <int32_t, 4>): return "int4";
	vcase(VectorType <uint32_t, 2>): return "uint2";
	vcase(VectorType <uint32_t, 3>): return "uint3";
	vcase(VectorType <uint32_t, 4>): return "uint4";
	vcase(VectorType <float, 2>): return "float2";
	vcase(VectorType <float, 3>): return "float3";
	vcase(VectorType <float, 4>): return "float4";

	vcase(MatrixType <int32_t, 2, 2>): return "int2x2";
	vcase(MatrixType <int32_t, 3, 3>): return "int3x3";
	vcase(MatrixType <int32_t, 4, 4>): return "int4x4";
	vcase(MatrixType <uint32_t, 2, 2>): return "uint2x2";
	vcase(MatrixType <uint32_t, 3, 3>): return "uint3x3";
	vcase(MatrixType <uint32_t, 4, 4>): return "uint4x4";
	vcase(MatrixType <float, 2, 2>): return "float2x2";
	vcase(MatrixType <float, 3, 3>): return "float3x3";
	vcase(MatrixType <float, 4, 4>): return "float4x4";
	default:
		break;
	}

	return "primitive(?)";
}

std::string stringify_type(AsmContext &ctx, AggregateType x, Reference ref)
{
	std::string result;
	for (size_t i = 0; i < x.size(); i++) {
		result += stringify(ctx, x[i]);
		if (i + 1 < x.size())
			result += ", ";
	}

	if (!x.name.empty())
		return x.name + "(" + result + ")";

	return "aggregate(" + result + ")";
}

std::string stringify_type(AsmContext &ctx, ArrayType x, Reference ref)
{
	return fmt::format("array({}, {})",
		stringify(ctx, x.base), x.size);
}

std::string stringify(AsmContext &ctx, Type x, Reference ref)
{
	return $assign std::visit([&](auto x) {
		return stringify_type(ctx, x, ref);
	}, x);
}

std::string stringify(AsmContext &ctx, Operation x, Reference ref)
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
	case OperationCode::eLogicalAnd: op = "and"; break;
	case OperationCode::eLogicalOr: op = "or"; break;
	case OperationCode::eLogicalXor: op = "xor"; break;
	case OperationCode::eLogicalNot:
		return $assign fmt::format("not({})", stringify(ctx, x.a));
	case OperationCode::eBitAnd: op = "band"; break;
	case OperationCode::eBitOr: op = "bor"; break;
	case OperationCode::eBitXor: op = "bxor"; break;
	case OperationCode::eBitNot:
		return $assign fmt::format("bnot({})", stringify(ctx, x.a));
	case OperationCode::eShiftLeft: op = "shl"; break;
	case OperationCode::eShiftRight: op = "shr"; break;
	default:
		break;
	}

	return $assign fmt::format("{}({}, {})",
		op, stringify(ctx, x.a), stringify(ctx, x.b));
}

std::string stringify(AsmContext &ctx, Store x, Reference ref)
{
	return fmt::format("store {} {}",
		stringify(ctx, x.destination), stringify(ctx, x.source));
}

std::string stringify(AsmContext &ctx, ArrayAccess x, Reference ref)
{
	return $assign fmt::format("index({}, {})",
		stringify(ctx, x.value), stringify(ctx, x.index));
}

std::string stringify(AsmContext &ctx, FieldAccess x, Reference ref)
{
	return $assign fmt::format("field {}:{}",
		stringify(ctx, x.value), x.fidx);
}

std::string stringify(AsmContext &ctx, Argument x, Reference ref)
{
	return $assign fmt::format("argument {}:{}",
		stringify(ctx, x.type), x.argi);
}

std::string stringify(AsmContext &ctx, GlobalResource x, Reference ref)
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
		stringify(ctx, x.type), opint(x.group), opint(x.index), repr(x.layout));
}

std::string stringify(AsmContext &ctx, ThreadInput x, Reference ref)
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

std::string stringify(AsmContext &ctx, ThreadOutput x, Reference ref)
{
	return $assign fmt::format("thread out({}, {}, {})",
		stringify(ctx, x.type), x.argi,
		stringify_rate_properties(x.properties));
}

std::string stringify(AsmContext &ctx, GlobalIntrinsic x, Reference ref)
{
	// TODO: use repr
	switch (x) {
	case GlobalIntrinsic::eClipPosition:
		return $assign "SVPosition";
	case GlobalIntrinsic::eLocalInvocationID:
		return $assign "LocalInvocationID";
	case GlobalIntrinsic::eWorkGroupID:
		return $assign "WorkGroupID";
	case GlobalIntrinsic::eGlobalInvocationID:
		return $assign "GlobalInvocationID";
	case GlobalIntrinsic::eTaskPayload:
		return $assign "TaskPayload";
	case GlobalIntrinsic::eMeshVertices:
		return $assign "MeshVertices";
	case GlobalIntrinsic::ePrimitiveTriangleIndices:
		return $assign "PrimitiveTriangleIndices";
	default:
		break;
	}

	return "?";
}

std::string stringify(AsmContext &ctx, Construct x, Reference ref)
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

std::string stringify(AsmContext &ctx, BuiltinIntrinsic x, Reference ref)
{
	// TODO: generate via a script by reading comments?
	// tagged with // @glsl abs
	std::string ftn = "?";
	switch (x.code) {
	case BuiltinIntrinsicCode::eAbs: ftn = "abs"; break;
	case BuiltinIntrinsicCode::eCross: ftn = "cross"; break;
	case BuiltinIntrinsicCode::eDFdx: ftn = "dFdx"; break;
	case BuiltinIntrinsicCode::eDFdy: ftn = "dFdy"; break;
	case BuiltinIntrinsicCode::eDFdxFine: ftn = "dFdxFine"; break;
	case BuiltinIntrinsicCode::eDFdyFine: ftn = "dFdyFine"; break;
	case BuiltinIntrinsicCode::eSample: ftn = "sample"; break;
	case BuiltinIntrinsicCode::eDot: ftn = "dot"; break;
	case BuiltinIntrinsicCode::eLength: ftn = "length"; break;
	case BuiltinIntrinsicCode::eSin: ftn = "sin"; break;
	case BuiltinIntrinsicCode::eToFloat: ftn = "float"; break;
	case BuiltinIntrinsicCode::eNormalize: ftn = "normalize"; break;
	case BuiltinIntrinsicCode::eMax: ftn = "max"; break;
	case BuiltinIntrinsicCode::ePow: ftn = "pow"; break;
	case BuiltinIntrinsicCode::eMin: ftn = "min"; break;
	case BuiltinIntrinsicCode::eSetMeshOutputsEXT: ftn = "SetMeshOutputsEXT"; break;
	case BuiltinIntrinsicCode::eEmitMeshTasksEXT: ftn = "EmitMeshTasksEXT"; break;
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

std::string stringify(AsmContext &ctx, Swizzle x, Reference ref)
{
	return $assign fmt::format("swizzle({}, {})",
		stringify(ctx, x.value), repr(x.code));
}

std::string stringify(AsmContext &ctx, Branch x, Reference ref)
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

std::string stringify(AsmContext &ctx, Loop x, Reference ref)
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

std::string stringify(AsmContext &ctx, Local x, Reference ref)
{
	return $assign fmt::format("local {}", stringify(ctx, x.type));
}

std::string stringify(AsmContext &ctx, Invocation x, Reference ref)
{
	std::string result;
	if (!x.sbr->name.empty())
		result = fmt::format("@{}(", x.sbr->name);
	else
		result = fmt::format("@{}(", (void *) x.sbr.get());
	
	for (size_t i = 0; i < x.args.size(); i++) {
		result += stringify(ctx, x.args[i]);
		if (i + 1 < x.args.size())
			result += ", ";
	}
	result += ")";

	return $assign result;
}

#undef $assign

std::string stringify(AsmContext &ctx, Block x, Reference ref)
{
	std::println(std::cerr, "cannot generate assembly for block");
	std::abort();
}

std::string stringify(ShaderStage stage)
{
	switch (stage) {
	case ShaderStage::eSubroutine: return "subroutine";
	case ShaderStage::eVertex: return "vertex shader";
	case ShaderStage::eFragment: return "fragment shader";
	case ShaderStage::eCompute: return "compute shader";
	case ShaderStage::eTask: return "task shader";
	case ShaderStage::eMesh: return "mesh shader";
	default:
		break;
	}

	return "?";
}

std::string generate_block_body(AsmContext &ctx, const SharedBlockReference &blk, const std::string &indent)
{
	std::string result = "block {\n";

	auto prefix = indent + "  ";
	auto pad_width = 40;
	if (indent.size() < pad_width)
		pad_width -= indent.size();

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

void emit_branch_block(AsmContext &ctx, const Branch &branch, Reference instr,
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
	result += ctx.auxiliary(line, instr->debug_info.origin);

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

void emit_loop_block(AsmContext &ctx, const Loop &loop, Reference instr,
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
	result += ctx.auxiliary(line, instr->debug_info.origin);

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

std::string generate(AsmContext &ctx)
{
	std::string result = "block {\n";

	result += "  context {\n";
	if (ctx.debug)
		result += fmt::format("    blkid: {},\n", (void *) ctx.sbr.get());

	result += "    model: " + stringify(ctx.sbr->model) + ",\n";
	if (!ctx.sbr->name.empty())
		result += fmt::format("    name: {},\n", ctx.sbr->name);

	for (auto arg : ctx.sbr->arguments) {
		result += fmt::format("    argument {}: {},\n",
			arg.argi, stringify(ctx, arg.type));
	}

	for (auto tin : ctx.sbr->thread_inputs) {
		result += fmt::format("    thread in {}: {},\n",
			tin.argi, stringify(ctx, tin.type));
	}

	for (auto tout : ctx.sbr->thread_outputs) {
		result += fmt::format("    thread out {}: {} ({}),\n",
			tout.argi, stringify(ctx, tout.type),
			stringify_rate_properties(tout.properties));
	}

	for (auto [k, v] : ctx.sbr->global_resources) {
		if (ctx.debug)
			result += fmt::format("    resource {}: {},\n", k, stringify(ctx, v));
		else
			result += fmt::format("    resource: {{{}}},\n", stringify(ctx, v));
	}

	result += "  }\n";

	for (auto &instr : *ctx.sbr) {
		if (instr->is <Branch> ()) {
			emit_branch_block(ctx,
				instr->as <Branch> (),
				instr, result
			);
			continue;
		}

		if (instr->is <Loop> ()) {
			emit_loop_block(ctx,
				instr->as <Loop> (),
				instr, result
			);
			continue;
		}

		auto str = std::visit([&](auto x) {
			return stringify(ctx, x, instr);
		}, *instr);

		result += ctx.auxiliary(str, instr->debug_info.origin);
	}
	result += "}";

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

std::string generate_assembly(const SharedBlockReference &sbr, bool debug)
{
	auto ctx = AsmContext {
		.sbr = sbr,
		.ids = {},
		.debug = debug,
	};

	return generate(ctx);
}

} // namespace rcgp
