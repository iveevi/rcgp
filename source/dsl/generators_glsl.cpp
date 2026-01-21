#include <array>
#include <cctype>
#include <string_view>
#include <fmt/format.h>

#include "dsl/generators.hpp"
#include "util/logging.hpp"
#include "util/timer.hpp"

struct Context {
	const Block &block;

	std::vector <std::string> thread_inputs;
	std::vector <std::string> argument_names;
	std::map <uint32_t, std::string> local_thread_outputs;
	std::map <const Instruction *, std::string> local_names;
	uint32_t local_count = 0;

	std::map <const AggregateType *, std::string> aggregate_names;
	std::map <const Block *, std::string> subroutine_names;
	std::set <const Block *> emitted_subroutines;

	const Block *active_block = nullptr;

	std::string result;
	std::string indent = "    ";
};

std::string sanitize_identifier(std::string_view name)
{
	std::string out;
	out.reserve(name.size());
	for (size_t i = 0; i < name.size(); i++) {
		unsigned char c = static_cast<unsigned char>(name[i]);
		if (c == ':' && i + 1 < name.size() && name[i + 1] == ':') {
			out.push_back('x');
			i++;
			continue;
		}
		if ((c >= 'a' && c <= 'z')
			|| (c >= 'A' && c <= 'Z')
			|| (c >= '0' && c <= '9')
			|| c == '_') {
			out.push_back(static_cast<char>(c));
		} else {
			out.push_back('_');
		}
	}

	if (out.empty())
		out = "AGG";

	if (out[0] >= '0' && out[0] <= '9')
		out.insert(out.begin(), 'T'), out.insert(out.begin() + 1, '_');

	return out;
}

std::string subroutine_name(Context &ctx, const Block *blk)
{
	auto it = ctx.subroutine_names.find(blk);
	if (it != ctx.subroutine_names.end())
		return it->second;

	std::string name = blk->context.name.empty()
		? fmt::format("fn_{}", (void *) blk)
		: blk->context.name;
	ctx.subroutine_names.emplace(blk, name);
	return name;
}

bool is_same_type(const Reference &a, const Reference &b)
{
	if (a.get() == b.get())
		return true;
	if (!a || !b)
		return false;
	if (!a->is <Type> () || !b->is <Type> ())
		return false;

	auto &ta = a->as <Type> ();
	auto &tb = b->as <Type> ();
	if (ta.index() != tb.index())
		return false;

	vswitch (ta) {
	vcase(PrimitiveType):
		return ta.as <PrimitiveType> ().index() == tb.as <PrimitiveType> ().index();
	vcase(AggregateType): {
		auto &aa = ta.as <AggregateType> ();
		auto &bb = tb.as <AggregateType> ();
		if (aa.size() != bb.size())
			return false;
		for (size_t i = 0; i < aa.size(); i++) {
			if (!is_same_type(aa[i], bb[i]))
				return false;
		}
		return true;
	}
	vcase(ArrayType): {
		auto &aa = ta.as <ArrayType> ();
		auto &bb = tb.as <ArrayType> ();
		if (aa.size != bb.size)
			return false;
		return is_same_type(aa.base, bb.base);
	}
	default:
		break;
	}

	return false;
}

std::string rate_qualifier(RateProperties properties)
{
	switch (properties) {
	case RateProperties::eFlat: return "flat ";
	case RateProperties::eNone: return "";
	case RateProperties::eNoPerspective: return "noperspective ";
	case RateProperties::eSmooth: return "smooth ";
	default:
		break;
	}

	return "? ";
}

std::string layout_string(GlobalResourceLayout layout)
{
	switch (layout) {
	case GlobalResourceLayout::eScalar: return "scalar, ";
	case GlobalResourceLayout::eStd430: return "std430, ";
	case GlobalResourceLayout::eNone:
	default:
		return "";
	}
}

// Type name generation
std::string primitive_type_string(const PrimitiveType &primitive)
{
	vswitch (primitive) {
	vcase(int32_t): return "int";
	vcase(uint32_t): return "uint";
	vcase(float): return "float";
	vcase(bool): return "bool";
	vcase(VectorType <float, 2>): return "vec2";
	vcase(VectorType <float, 3>): return "vec3";
	vcase(VectorType <float, 4>): return "vec4";
	vcase(VectorType <int32_t, 2>): return "ivec2";
	vcase(VectorType <int32_t, 3>): return "ivec3";
	vcase(VectorType <int32_t, 4>): return "ivec4";
	vcase(VectorType <uint32_t, 2>): return "uvec2";
	vcase(VectorType <uint32_t, 3>): return "uvec3";
	vcase(VectorType <uint32_t, 4>): return "uvec4";
	vcase(MatrixType <int32_t, 2, 2>): return "imat2";
	vcase(MatrixType <int32_t, 3, 3>): return "imat3";
	vcase(MatrixType <int32_t, 4, 4>): return "imat4";
	vcase(MatrixType <uint32_t, 2, 2>): return "umat2";
	vcase(MatrixType <uint32_t, 3, 3>): return "umat3";
	vcase(MatrixType <uint32_t, 4, 4>): return "umat4";
	vcase(MatrixType <float, 2, 2>): return "mat2";
	vcase(MatrixType <float, 3, 3>): return "mat3";
	vcase(MatrixType <float, 4, 4>): return "mat4";
	default:
		break;
	}

	fatal("unhandled primitive type string case");
}

struct TypeRepr {
	std::string base;
	std::string suffix;
};

TypeRepr type_repr(Context &ctx, const Reference &ref)
{
	// TODO: each reference should have an assembly dump (with tabs);
	// in fact we should overhaul the current assembly generator
	// to be like this...
	assertion(ref->is <Type> (),
		"type_repr only operates on Type instructions:"
		"\n\tref: %s",
		ref->repr().c_str());

	auto &type = ref->as <Type> ();
	vswitch (type) {
	vcase(PrimitiveType): {
		auto &pt = type.as <PrimitiveType> ();
		auto ptstr = primitive_type_string(pt);
		return TypeRepr { ptstr, "" };
	}
	vcase(AggregateType): {
		auto &agg = type.as <AggregateType> ();
		auto name = ctx.aggregate_names.at(&agg);
		return TypeRepr { name,  "" };
	}
	vcase(ArrayType): {
		auto &array = type.as <ArrayType> ();
		auto decl = type_repr(ctx, array.base);
		decl.suffix += (array.size > 0)
			? fmt::format("[{}]", array.size)
			: "[]";
		return decl;
	}
	default:
		break;
	}

	fatal("unhandled case for type_repr:\n%s", ref->repr().c_str());
}

bool contains_unsized_array(Reference type)
{
	if (not type or not type->is <Type> ())
		return false;

	auto &t = type->as <Type> ();
	if (t.is <ArrayType> ()) {
		auto &arr = t.as <ArrayType> ();
		if (arr.size <= 0)
			return true;
		return contains_unsized_array(arr.base);
	}

	if (t.is <AggregateType> ()) {
		auto &agg = t.as <AggregateType> ();
		for (auto &field : agg) {
			if (contains_unsized_array(field))
				return true;
		}
	}

	return false;
}

bool contains_unsized_array(const AggregateType &agg)
{
	for (auto &field : agg) {
		if (contains_unsized_array(field))
			return true;
	}
	return false;
}

std::string reference_local(Context &ctx, Reference ref)
{
	auto ptr = ref.get();
	auto it = ctx.local_names.find(ptr);
	if (it != ctx.local_names.end())
		return it->second;

	auto name = fmt::format("lvar{}", ctx.local_count++);
	ctx.local_names.emplace(ptr, name);
	return name;
}

std::string resource_base_name(const GlobalResource &grsrc)
{
	auto group = grsrc.group.value_or(0);
	auto index = grsrc.index.value_or(0);
	if (grsrc.kind == GlobalResourceKind::eSampler)
		return fmt::format("s{}_i{}", group, index);
	return fmt::format("r{}_i{}", group, index);
}

std::string reference(Context &ctx, GlobalIntrinsic gi)
{
	switch (gi) {
	case GlobalIntrinsic::eClipPosition: return "gl_Position";
	case GlobalIntrinsic::eInstanceIndex: return "gl_InstanceIndex";
	case GlobalIntrinsic::eLocalInvocationID: return "gl_LocalInvocationID";
	case GlobalIntrinsic::eWorkGroupID: return "gl_WorkGroupID";
	case GlobalIntrinsic::eGlobalInvocationID: return "gl_GlobalInvocationID";
	case GlobalIntrinsic::eTaskPayload: return "task_payload";
	case GlobalIntrinsic::eMeshVertices: return "gl_MeshVerticesEXT";
	case GlobalIntrinsic::ePrimitiveTriangleIndices: return "gl_PrimitiveTriangleIndicesEXT";
	default:
		return "?";
	}
}

std::string reference(Context &ctx, GlobalResource grsrc)
{
	if (grsrc.kind == GlobalResourceKind::ePushConstant) {
		auto idx = grsrc.push_constant_index.value_or(0);
		return fmt::format("pc{}", idx);
	}

	auto base = resource_base_name(grsrc);
	if (grsrc.kind == GlobalResourceKind::eSampler)
		return base;

	if (!grsrc.type || !grsrc.type->is <Type> ())
		return base;

	auto &type = grsrc.type->as <Type> ();
	if (type.is <AggregateType> ())
		return base;

	return base + ".value";
}

std::string reference(Context &ctx, ThreadOutput tout)
{
	if (ctx.active_block
		&& ctx.active_block->context.model == ShaderStage::eSubroutine
		&& ctx.local_thread_outputs.contains(tout.argi)) {
		return ctx.local_thread_outputs[tout.argi];
	}

	return fmt::format("lout{}", tout.argi);
}

std::string reference(Context &ctx, Argument arg)
{
	if (arg.argi < ctx.argument_names.size())
		return ctx.argument_names[arg.argi];
	return fmt::format("arg{}", arg.argi);
}

std::string expression(Context &ctx, Reference expr);

// TODO: rebrand to lvalue
std::string reference(Context &ctx, Reference ref)
{
	auto &value = *ref;
	vswitch (value) {
	vcase(Local):
		return reference_local(ctx, ref);
	vcase(ArrayAccess):
		return expression(ctx, ref);
	vcase(FieldAccess):
		return expression(ctx, ref);
	vcase(Swizzle):
		return expression(ctx, ref);
	vcase(GlobalIntrinsic):
		return reference(ctx, value.as <GlobalIntrinsic> ());
	vcase(GlobalResource):
		return reference(ctx, value.as <GlobalResource> ());
	vcase(ThreadOutput):
		return reference(ctx, value.as <ThreadOutput> ());
	vcase(Argument):
		return reference(ctx, value.as <Argument> ());
	default:
		break;
	}

	fatal("unhandled reference:\n%s", ref->repr().c_str());
}

std::string expression(Context &ctx, Reference expr)
{
	auto &value = *expr;
	vswitch (value) {
	vcase(Local):
		return reference(ctx, expr);
	vcase(Constant): {
		auto &constant = value.as <Constant> ();
		if (constant.template is <int32_t> ())
			return fmt::format("{}", constant.template as <int32_t> ());
		if (constant.template is <uint32_t> ())
			return fmt::format("{}", constant.template as <uint32_t> ());
		if (constant.template is <bool> ())
			return fmt::format("{}", constant.template as <bool> ());
		if (constant.template is <float> ())
			return fmt::format("{}", constant.template as <float> ());
		return "?";
	}
	vcase(Operation): {
		auto &op = value.as <Operation> ();
		if (op.code == OperationCode::eLogicalNot) {
			return fmt::format("(!{})", expression(ctx, op.a));
		}
		if (op.code == OperationCode::eBitNot) {
			return fmt::format("(~{})", expression(ctx, op.a));
		}

		std::string op_name = "?";
		switch (op.code) {
		case OperationCode::eAdd: op_name = "+"; break;
		case OperationCode::eSubtract: op_name = "-"; break;
		case OperationCode::eMultiply: op_name = "*"; break;
		case OperationCode::eDivide: op_name = "/"; break;
		case OperationCode::eEqual: op_name = "=="; break;
		case OperationCode::eNotEqual: op_name = "!="; break;
		case OperationCode::eLess: op_name = "<"; break;
		case OperationCode::eLessEqual: op_name = "<="; break;
		case OperationCode::eGreater: op_name = ">"; break;
		case OperationCode::eGreaterEqual: op_name = ">="; break;
		case OperationCode::eLogicalAnd: op_name = "&&"; break;
		case OperationCode::eLogicalOr: op_name = "||"; break;
		case OperationCode::eLogicalXor: op_name = "^^"; break;
		case OperationCode::eBitAnd: op_name = "&"; break;
		case OperationCode::eBitOr: op_name = "|"; break;
		case OperationCode::eBitXor: op_name = "^"; break;
		case OperationCode::eShiftLeft: op_name = "<<"; break;
		case OperationCode::eShiftRight: op_name = ">>"; break;
		default:
			break;
		}

		return fmt::format("({} {} {})",
			expression(ctx, op.a), op_name, expression(ctx, op.b));
	}
	vcase(ThreadInput): {
		auto &input = value.as <ThreadInput> ();
		return ctx.thread_inputs[input.argi];
	}
	vcase(Construct): {
		auto &construct = value.as <Construct> ();
	
		// TODO: need to detect arrays; also should
		// handle static (global) construction
		auto repr = type_repr(ctx, construct.type);
		assertion(repr.suffix.empty(),
			"construct cannot handle array types yet (base=%p, suffix=%p)",
			repr.base.c_str(),
			repr.suffix.c_str()
		);

		std::string out = repr.base + "(";

		auto nargs = construct.args.size();
		for (size_t i = 0; i < nargs; i++) {
			out += expression(ctx, construct.args[i]);
			if (i + 1 < nargs)
				out += ", ";
		}

		return out + ")";
	}
	vcase(FieldAccess): {
		auto &access = value.as <FieldAccess> ();
		return fmt::format("{}.f{}", expression(ctx, access.value), access.fidx);
	}
	vcase(ArrayAccess): {
		auto &access = value.as <ArrayAccess> ();
		if (access.value && access.value->is <GlobalIntrinsic> ()) {
			auto gi = access.value->as <GlobalIntrinsic> ();
			if (gi == GlobalIntrinsic::eMeshVertices) {
				return fmt::format("gl_MeshVerticesEXT[{}].gl_Position",
					expression(ctx, access.index));
			}
		}
		return fmt::format("{}[{}]",
			expression(ctx, access.value), expression(ctx, access.index));
	}
	vcase(Swizzle): {
		auto &swizzle = value.as <Swizzle> ();
		return fmt::format("{}.{}", expression(ctx, swizzle.value), swizzle_string(swizzle.code));
	}
	vcase(GlobalResource):
		return reference(ctx, value.as <GlobalResource> ());
	vcase(GlobalIntrinsic):
		return reference(ctx, value.as <GlobalIntrinsic> ());
	vcase(ThreadOutput):
		return reference(ctx, value.as <ThreadOutput> ());
	vcase(Argument):
		return reference(ctx, value.as <Argument> ());
	vcase(Invocation): {
		auto &invocation = value.as <Invocation> ();
		const Block *callee = invocation.sbr.get();
		std::string name = subroutine_name(ctx, callee);

		std::string out = name + "(";
		auto nargs = invocation.args.size();
		for (size_t i = 0; i < nargs; i++) {
			out += expression(ctx, invocation.args[i]);
			if (i + 1 < nargs)
				out += ", ";
		}

		return out + ")";
	}
	vcase(BuiltinIntrinsic): {
		auto &intrinsic = value.as <BuiltinIntrinsic> ();
		std::string out = "?";

		switch (intrinsic.code) {
		case BuiltinIntrinsicCode::eCross: out = "cross"; break;
		case BuiltinIntrinsicCode::eDFdx: out = "dFdx"; break;
		case BuiltinIntrinsicCode::eDFdxFine: out = "dFdxFine"; break;
		case BuiltinIntrinsicCode::eDFdy: out = "dFdy"; break;
		case BuiltinIntrinsicCode::eDFdyFine: out = "dFdyFine"; break;
		case BuiltinIntrinsicCode::eDot: out = "dot"; break;
		case BuiltinIntrinsicCode::eLength: out = "length"; break;
		case BuiltinIntrinsicCode::eSin: out = "sin"; break;
		case BuiltinIntrinsicCode::eToFloat: out = "float"; break;
		case BuiltinIntrinsicCode::eInverse: out = "inverse"; break;
		case BuiltinIntrinsicCode::eMax: out = "max"; break;
		case BuiltinIntrinsicCode::ePow: out = "pow"; break;
		case BuiltinIntrinsicCode::eMin: out = "min"; break;
		case BuiltinIntrinsicCode::eNormalize: out = "normalize"; break;
		case BuiltinIntrinsicCode::eSample: out = "texture"; break;
		case BuiltinIntrinsicCode::eTranspose: out = "transpose"; break;
		case BuiltinIntrinsicCode::eSetMeshOutputsEXT: out = "SetMeshOutputsEXT"; break;
		case BuiltinIntrinsicCode::eEmitMeshTasksEXT: out = "EmitMeshTasksEXT"; break;
		default:
			break;
		}

		out += "(";

		auto nargs = intrinsic.args.size();
		for (size_t i = 0; i < nargs; i++) {
			out += expression(ctx, intrinsic.args[i]);
			if (i + 1 < nargs)
				out += ", ";
		}

		return out + ")";
	}
	default:
		break;
	}

	fatal("expression not implemented for %s", expr->repr().c_str());
	return "?";
}

void statement(Context &ctx, Reference instruction)
{
	auto &value = *instruction;
	vswitch (value) {
	vcase(Local): {
		auto name = reference(ctx, instruction);
		auto decl = type_repr(ctx, value.as <Local> ().type);
		ctx.result += fmt::format("{}{} {}{};\n",
			ctx.indent, decl.base, name, decl.suffix);
		return;
	}
	vcase(Store): {
		auto &store = value.as <Store> ();
		ctx.result += fmt::format("{}{} = {};\n",
			ctx.indent,
			reference(ctx, store.destination),
			expression(ctx, store.source));
		return;
	}
	vcase(BuiltinIntrinsic):
		ctx.result += fmt::format("{}{};\n",
			ctx.indent,
			expression(ctx, instruction));
		return;
	vcase(Invocation):
		ctx.result += fmt::format("{}{};\n",
			ctx.indent,
			expression(ctx, instruction));
		return;
	vcase(Branch): {
		auto &branch = value.as <Branch> ();
		auto emit_body = [&](const SharedBlockReference &blk) {
			auto prev = ctx.indent;
			ctx.indent += "    ";
			for (auto &nested : *blk) {
				if (nested->is <Store> () || nested->is <Invocation> ()
					|| nested->is <BuiltinIntrinsic> ()
					|| nested->is <Branch> () || nested->is <Loop> ()
					|| nested->is <Local> ())
					statement(ctx, nested);
			}
			ctx.indent = prev;
		};

		for (size_t i = 0; i < branch.segments.size(); i++) {
			auto &segment = branch.segments[i];
			auto kw = (i == 0) ? "if" : "else if";
			ctx.result += fmt::format("{}{} ({})\n",
				ctx.indent, kw, expression(ctx, segment.cond));
			ctx.result += fmt::format("{}{{\n", ctx.indent);
			emit_body(segment.body);
			ctx.result += fmt::format("{}}}\n", ctx.indent);
		}

		if (branch.fallback.has_value()) {
			ctx.result += fmt::format("{}else\n", ctx.indent);
			ctx.result += fmt::format("{}{{\n", ctx.indent);
			emit_body(branch.fallback.value());
			ctx.result += fmt::format("{}}}\n", ctx.indent);
		}
		return;
	}
	vcase(Loop): {
		auto &loop = value.as <Loop> ();
		auto emit_body = [&](const SharedBlockReference &blk) {
			auto prev = ctx.indent;
			ctx.indent += "    ";
			for (auto &nested : *blk) {
				if (nested->is <Store> () || nested->is <Invocation> ()
					|| nested->is <BuiltinIntrinsic> ()
					|| nested->is <Branch> () || nested->is <Loop> ()
					|| nested->is <Local> ())
					statement(ctx, nested);
			}
			ctx.indent = prev;
		};

		auto cond_expr = expression(ctx, loop.cond_value);

		if (loop.kind == LoopKind::eFor && loop.init.has_value())
			emit_body(loop.init.value());

		auto loop_kw = (loop.kind == LoopKind::eFor) ? "for (;;)" : "while (true)";
		ctx.result += fmt::format("{}{}\n", ctx.indent, loop_kw);
		ctx.result += fmt::format("{}{{\n", ctx.indent);
		emit_body(loop.cond);
		ctx.result += fmt::format("{}    if (!({}))\n", ctx.indent, cond_expr);
		ctx.result += fmt::format("{}        break;\n", ctx.indent);
		emit_body(loop.body);
		if (loop.kind == LoopKind::eFor && loop.step.has_value())
			emit_body(loop.step.value());
		ctx.result += fmt::format("{}}}\n", ctx.indent);
		return;
	}
	default:
		break;
	}

	ctx.result += fmt::format("{}?\n", ctx.indent);
}

void emit_block_statements(Context &ctx, const Block &blk)
{
	for (auto &instr : blk) {
		if (instr->is <Store> ()
			|| instr->is <Invocation> ()
			|| instr->is <BuiltinIntrinsic> ()
			|| instr->is <Branch> () || instr->is <Loop> ()
			|| instr->is <Local> ())
			statement(ctx, instr);
	}
}

void set_active_block(Context &ctx, const Block &blk)
{
	ctx.active_block = &blk;
}

void reset_state(Context &ctx)
{
	ctx.result.clear();
	ctx.aggregate_names.clear();
	ctx.subroutine_names.clear();
	ctx.emitted_subroutines.clear();
	ctx.thread_inputs.clear();
	ctx.argument_names.clear();
	ctx.local_thread_outputs.clear();
	ctx.local_names.clear();
	ctx.local_count = 0;
	set_active_block(ctx, ctx.block);
}

void collect_blocks(const Context &ctx, std::vector <const Block *> &blocks)
{
	std::set <const Block *> visited;

	auto visit = [&](auto &&self, const Block &blk) -> void {
		if (visited.contains(&blk))
			return;
		visited.emplace(&blk);
		blocks.push_back(&blk);

		for (auto &instr : blk) {
			if (instr->is <Invocation> ()) {
				auto &inv = instr->as <Invocation> ();
				self(self, *inv.sbr);
			} else if (instr->is <Branch> ()) {
				auto &branch = instr->as <Branch> ();
				for (auto &segment : branch.segments)
					self(self, *segment.body);
				if (branch.fallback.has_value())
					self(self, *branch.fallback.value());
			} else if (instr->is <Loop> ()) {
				auto &loop = instr->as <Loop> ();
				if (loop.init.has_value())
					self(self, *loop.init.value());
				self(self, *loop.cond);
				if (loop.step.has_value())
					self(self, *loop.step.value());
				self(self, *loop.body);
			}
		}
	};

	visit(visit, ctx.block);
}

void emit_preamble(Context &ctx)
{
	ctx.result = "// Preamble\n";
	ctx.result += "#version 460\n\n";
	ctx.result += "#extension GL_EXT_scalar_block_layout : require\n";
	if (ctx.block.context.model == ShaderStage::eTask
		|| ctx.block.context.model == ShaderStage::eMesh) {
		ctx.result += "#extension GL_EXT_mesh_shader : require\n";
	}
	ctx.result += "\n";
	if (ctx.block.context.model == ShaderStage::eCompute
		|| ctx.block.context.model == ShaderStage::eTask
		|| ctx.block.context.model == ShaderStage::eMesh) {
		auto size = ctx.block.context.workgroup_size.value_or(
			std::array <uint32_t, 3> { 1, 1, 1 }
		);
		ctx.result += fmt::format(
			"layout (local_size_x = {}, local_size_y = {}, local_size_z = {}) in;\n\n",
			size[0], size[1], size[2]
		);
	}
	if (ctx.block.context.model == ShaderStage::eMesh) {
		if (!ctx.block.context.mesh_max_vertices.has_value()
			|| !ctx.block.context.mesh_max_primitives.has_value())
			fatal("mesh shader missing MeshletPayload size information");
		auto max_vertices = ctx.block.context.mesh_max_vertices.value();
		auto max_primitives = ctx.block.context.mesh_max_primitives.value();
		ctx.result += fmt::format(
			"layout (max_vertices = {}, max_primitives = {}) out;\n",
			max_vertices, max_primitives
		);
		ctx.result += "layout (triangles) out;\n\n";
	}
}

void emit_aggregate_decls(Context &ctx)
{
	ctx.result += "// Types\n";

	// TODO: this pattern is common enough,
	// make an abstraction out of it with
	// an instruction emitter
	std::vector <const Block *> blocks;
	collect_blocks(ctx, blocks);

	size_t aggregate_counter = 0;
	std::map <std::string, size_t> name_counts;
	std::set <const AggregateType *> emitted;
	for (auto *blk : blocks) {
		for (auto &instr : *blk) {
			if (not instr->is <Type> ())
				continue;

			auto &type = instr->as <Type> ();
			if (not type.is <AggregateType> ())
				continue;

			auto &agg = type.as <AggregateType> ();
			if (emitted.contains(&agg))
				continue;
			if (contains_unsized_array(agg))
				continue;

			auto base = agg.name.empty()
				? fmt::format("AGG{}", aggregate_counter)
				: sanitize_identifier(agg.name);
			auto &count = name_counts[base];
			auto name = (count == 0) ? base : fmt::format("{}_{}", base, count);
			count++;

			ctx.result += fmt::format("struct {} {{\n", name);

			size_t field_counter = 0;
			for (auto &field : agg) {
				auto decl = type_repr(ctx, field);
				ctx.result += fmt::format("    {} f{}{};\n",
					decl.base, field_counter++, decl.suffix);
			}

			ctx.result += "};\n\n";

			ctx.aggregate_names.emplace(&agg, name);
			aggregate_counter++;
			emitted.emplace(&agg);
		}
	}

	ctx.result += "\n";
}

void emit_task_payload(Context &ctx)
{
	if (!ctx.block.context.task_payload_type.has_value())
		return;

	auto decl = type_repr(ctx, ctx.block.context.task_payload_type.value());
	ctx.result += "// Task Payload\n";
	ctx.result += fmt::format("taskPayloadSharedEXT {} task_payload{};\n\n",
		decl.base, decl.suffix);
}

void emit_thread_inputs(Context &ctx)
{
	ctx.result += "// Inputs\n";

	ctx.thread_inputs.reserve(ctx.block.context.thread_inputs.size());
	for (auto &tin : ctx.block.context.thread_inputs) {
		ctx.thread_inputs.push_back(fmt::format("lin{}", tin.argi));
		auto decl = type_repr(ctx, tin.type);
		ctx.result += fmt::format("layout (location = {}) in {} lin{}{};\n",
			tin.argi, decl.base, tin.argi, decl.suffix);
	}

	if (ctx.block.context.thread_inputs.size())
		ctx.result += "\n";
	else
		ctx.result += "\n";
}

void emit_thread_outputs(Context &ctx)
{
	ctx.result += "// Outputs\n";

	std::map <uint32_t, ThreadOutput> outputs;
	std::vector <const Block *> blocks;
	collect_blocks(ctx, blocks);

	for (auto *blk : blocks) {
		for (auto &tout : blk->context.thread_outputs) {
			auto it = outputs.find(tout.argi);
			if (it == outputs.end()) {
				outputs.emplace(tout.argi, tout);
				continue;
			}

			if (not is_same_type(it->second.type, tout.type)) {
				fatal("thread output type mismatch for location %u (%s vs %s)",
					tout.argi,
					it->second.type->repr().c_str(),
					tout.type->repr().c_str());
			}

			if (it->second.properties != tout.properties) {
				fatal("thread output properties mismatch for lcoation %u (%d vs %d)",
					tout.argi, it->second.properties, tout.properties);
			}
		}
	}

	for (auto &[_, tout] : outputs) {
		auto qualifier = rate_qualifier(tout.properties);
		if (ctx.block.context.model == ShaderStage::eMesh) {
			auto it = ctx.block.context.mesh_perprimitive_outputs.find(tout.argi);
			if (it != ctx.block.context.mesh_perprimitive_outputs.end() && it->second)
				qualifier = qualifier.empty() ? "perprimitiveEXT " : qualifier + " perprimitiveEXT ";
		}
		auto decl = type_repr(ctx, tout.type);
		ctx.result += fmt::format("layout (location = {}) {}out {} lout{}{};\n",
			tout.argi, qualifier, decl.base, tout.argi, decl.suffix);
	}

	if (outputs.size())
		ctx.result += "\n";
	else
		ctx.result += "\n";
}

void collect_push_constant_indices(
	const std::vector <const Block *> &blocks,
	std::map <void *, uint32_t> &pc_indices
)
{
	uint32_t pcounter = 0;
	for (auto *blk : blocks) {
		for (auto &[addr, refs] : blk->context.global_resources) {
			for (auto &ref : refs) {
				auto &grsrc = ref->as <GlobalResource> ();
				if (grsrc.kind != GlobalResourceKind::ePushConstant)
					continue;

				auto it = pc_indices.find(addr);
				if (it == pc_indices.end())
					it = pc_indices.emplace(addr, pcounter++).first;
				grsrc.push_constant_index = it->second;
			}
		}
	}
}

std::string resource_key(const GlobalResource &grsrc)
{
	if (grsrc.kind == GlobalResourceKind::eSampler) {
		auto group = grsrc.group.value_or(0);
		auto index = grsrc.index.value_or(0);
		return fmt::format("sampler:{}:{}", group, index);
	}

	if (grsrc.kind == GlobalResourceKind::ePushConstant) {
		auto idx = grsrc.push_constant_index.value_or(0);
		auto offset = grsrc.push_constant_offset.value_or(0);
		return fmt::format("pc:{}:{}:{}:{}",
			idx, offset, (int) grsrc.layout, (void *) grsrc.type.get());
	}

	auto group = grsrc.group.value_or(0);
	auto index = grsrc.index.value_or(0);
	return fmt::format("buf:{}:{}:{}:{}:{}",
		(int) grsrc.kind, group, index, (int) grsrc.layout, (int) grsrc.access);
}

std::string resource_instance_name(const GlobalResource &grsrc)
{
	return resource_base_name(grsrc);
}

void emit_buffer_fields(Context &ctx, const AggregateType &agg)
{
	for (size_t i = 0; i < agg.size(); i++) {
		if (contains_unsized_array(agg[i]) && (i + 1 < agg.size()))
			fatal("unsized array must be the last field in a buffer block");
		auto decl = type_repr(ctx, agg[i]);
		ctx.result += fmt::format("    {} f{}{};\n", decl.base, i, decl.suffix);
	}
}

void emit_resource_decl(Context &ctx, GlobalResource &grsrc)
{
	if (grsrc.kind == GlobalResourceKind::eSampler) {
		auto group = grsrc.group.value_or(0);
		auto index = grsrc.index.value_or(0);
		ctx.result += fmt::format("layout (set = {}, binding = {}) uniform sampler2D {};\n\n",
			group, index, resource_base_name(grsrc));
		return;
	}

	if (grsrc.kind == GlobalResourceKind::ePushConstant) {
		auto idx = grsrc.push_constant_index.value_or(0);
		grsrc.push_constant_index = idx;
		auto offset = grsrc.push_constant_offset.value_or(0);
		auto decl = type_repr(ctx, grsrc.type);

		ctx.result += fmt::format("layout ({}push_constant) uniform PC{} {{\n",
			layout_string(grsrc.layout), idx);
		ctx.result += fmt::format("    layout (offset = {}) {} pc{}{};\n",
			offset, decl.base, idx, decl.suffix);
		ctx.result += "};\n\n";
		return;
	}

	std::string modifier;
	switch (grsrc.kind) {
	case GlobalResourceKind::eUniformBuffer: modifier = "uniform"; break;
	case GlobalResourceKind::eStorageBuffer:
		switch (grsrc.access) {
		case GlobalResourceAccess::eRead: modifier = "readonly buffer"; break;
		case GlobalResourceAccess::eWrite: modifier = "writeonly buffer"; break;
		case GlobalResourceAccess::eReadWrite: modifier = "buffer"; break;
		default: modifier = "buffer"; break;
		}
		break;
	default:
		fatal("unsupported global resource kind");
	}

	auto group = grsrc.group.value_or(0);
	auto index = grsrc.index.value_or(0);
	auto instance = resource_instance_name(grsrc);
	ctx.result += fmt::format("layout ({}set = {}, binding = {}) {} R{}{} {{\n",
		layout_string(grsrc.layout), group, index, modifier, group, index);
	if (!grsrc.type || !grsrc.type->is <Type> ())
		fatal("invalid resource type");

	auto &type = grsrc.type->as <Type> ();
	if (type.is <AggregateType> ()) {
		emit_buffer_fields(ctx, type.as <AggregateType> ());
	} else {
		auto decl = type_repr(ctx, grsrc.type);
		ctx.result += fmt::format("    {} value{};\n", decl.base, decl.suffix);
	}
	ctx.result += fmt::format("}} {};\n\n", instance);
}

void emit_global_resources(Context &ctx)
{
	std::vector <const Block *> blocks;
	collect_blocks(ctx, blocks);

	std::map <void *, uint32_t> pc_indices;
	collect_push_constant_indices(blocks, pc_indices);

	ctx.result += "// Resources\n";

	std::set <std::string> emitted;
	for (auto *blk : blocks) {
		for (auto &[_, refs] : blk->context.global_resources) {
			for (auto &ref : refs) {
				auto &grsrc = ref->as <GlobalResource> ();
				auto key = resource_key(grsrc);
				if (emitted.contains(key))
					continue;
				emitted.emplace(key);
				emit_resource_decl(ctx, grsrc);
			}
		}
	}

	ctx.result += "\n";
}

std::string subroutine_return_type(Context &ctx, const Block &blk, uint32_t &out_argi)
{
	// TODO: should have proper returns (and proper parameters instead of thread index)
	out_argi = 0;
	if (blk.context.thread_outputs.empty())
		return "void";
	if (blk.context.thread_outputs.size() > 1)
		fatal("subroutine return with multiple outputs is not supported");

	auto &tout = blk.context.thread_outputs.front();
	out_argi = tout.argi;

	auto repr = type_repr(ctx, tout.type);
	return repr.base + repr.suffix;
}

void emit_subroutine_function(Context &ctx, const Block &blk, const std::string &name)
{
	set_active_block(ctx, blk);
	ctx.argument_names.clear();
	ctx.local_thread_outputs.clear();

	for (auto &arg : blk.context.arguments)
		ctx.argument_names.push_back(fmt::format("arg{}", arg.argi));

	uint32_t ret_argi = 0;
	auto return_type = subroutine_return_type(ctx, blk, ret_argi);

	ctx.result += fmt::format("{} {}(", return_type, name);
	for (size_t i = 0; i < blk.context.arguments.size(); i++) {
		auto &arg = blk.context.arguments[i];
		auto decl = type_repr(ctx, arg.type);
		ctx.result += fmt::format("{} {}{}", decl.base, ctx.argument_names[arg.argi], decl.suffix);
		if (i + 1 < blk.context.arguments.size())
			ctx.result += ", ";
	}
	ctx.result += ")\n{\n";

	// TODO: should just be one set of returns...
	// for perf maybe even prefer out parameters for tuples
	for (auto &tout : blk.context.thread_outputs) {
		auto name = fmt::format("lout{}", tout.argi);
		ctx.local_thread_outputs.emplace(tout.argi, name);
		auto decl = type_repr(ctx, tout.type);
		ctx.result += fmt::format("    {} {}{};\n", decl.base, name, decl.suffix);
	}

	if (blk.context.thread_outputs.size())
		ctx.result += "\n";

	emit_block_statements(ctx, blk);

	if (return_type != "void") {
		auto name_it = ctx.local_thread_outputs.find(ret_argi);
		auto local_name = (name_it != ctx.local_thread_outputs.end())
			? name_it->second
			: fmt::format("lout{}", ret_argi);
		ctx.result += fmt::format("    return {};\n", local_name);
	}

	ctx.result += "}\n\n";
	set_active_block(ctx, ctx.block);
}

void emit_subroutine_functions(Context &ctx)
{
	ctx.result += "// Subroutines\n";

	std::vector <const Block *> order;
	std::set <const Block *> visited;

	auto visit = [&](auto &&self, const Block &blk) -> void {
		for (auto &instr : blk) {
			if (instr->is <Invocation> ()) {
				auto &inv = instr->as <Invocation> ();
				const Block *callee = inv.sbr.get();
				if (visited.contains(callee))
					continue;
				visited.emplace(callee);
				self(self, *callee);
				order.push_back(callee);
			}
		}
	};

	visit(visit, ctx.block);

	for (auto *callee : order) {
		if (callee->context.model != ShaderStage::eSubroutine)
			continue;

		auto name = subroutine_name(ctx, callee);

		if (ctx.emitted_subroutines.contains(callee))
			continue;

		ctx.emitted_subroutines.emplace(callee);
		emit_subroutine_function(ctx, *callee, name);
	}
}

void emit_main_function(Context &ctx)
{
	ctx.result += "// Main\n";
	ctx.result += "void main()\n";
	ctx.result += "{\n";

	emit_block_statements(ctx, ctx.block);

	ctx.result += "}";
}

std::string generate(Context &ctx, size_t tabs)
{
	reset_state(ctx);

	if (ctx.block.context.model == ShaderStage::eSubroutine) {
		emit_aggregate_decls(ctx);
		auto name = subroutine_name(ctx, &ctx.block);
		emit_subroutine_function(ctx, ctx.block, name);
		return ctx.result;
	}

	emit_preamble(ctx);
	emit_aggregate_decls(ctx);
	emit_task_payload(ctx);
	emit_thread_inputs(ctx);
	emit_thread_outputs(ctx);
	emit_global_resources(ctx);
	emit_subroutine_functions(ctx);
	emit_main_function(ctx);

	return ctx.result;
}

std::string generate_glsl(const SharedBlockReference &sbr, size_t tabs)
{
	TSCOPE("generating glsl code");
	Context ctx { *sbr.get() };
	return generate(ctx, tabs);
}
