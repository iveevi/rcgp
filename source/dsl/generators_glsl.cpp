#include <iostream>
#include <print>
#include <set>

#include "dsl/generators.hpp"
#include "dsl/instructions.hpp"
#include "util/timer.hpp"

#undef assert
#define assert(c) 							\
	if (not (c)) {							\
		std::println(std::cerr, "assertion failed: {}", #c);	\
		std::flush(std::cerr);					\
		__builtin_trap();					\
	}

#define fatal(...) {							\
		std::println(std::cerr, __VA_ARGS__);			\
		std::flush(std::cerr);					\
		__builtin_trap();					\
	}

namespace rcgp {

// TODO: separate cpp file for this...
static auto g_rate_strings = std::array {
	// "?",
	"", // TODO: sync the rate properties from outputs of vs/mesh and fragment shader?
	"smooth",
	"flat",
	"noperspective",
};

static auto g_resource_layouts = std::array {
	"?",
	"scalar",
	"std430",
};

static auto g_global_intrinsics = std::array {
	// TODO: generate with script with @glsl comments
	"gl_Position",
	"gl_InstanceIndex",
	"gl_LocalInvocationID",
	"gl_WorkGroupID",
	"gl_GlobalInvocationID",
	"task_payload",
	"gl_MeshVerticesEXT",
	"gl_PrimitiveTriangleIndicesEXT",
};

static auto g_operation_code = std::array {
	"+",
	"-",
	"*",
	"/",
	"==",
	"!=",
	"<",
	"<=",
	">",
	">=",
	"&&",
	"||",
	"^",
	"!",
	"&",
	"|",
	"^",
	"~",
	"<<",
	">>",
};

static auto g_intrinsic_code = std::array {
	// TODO: also use @glsl trick...
	"abs",
	"cos",
	"cross",
	"dFdx",
	"dFdxFine",
	"dFdy",
	"dFdyFine",
	"dot",
	"inverse",
	"length",
	"max",
	"pow",
	"float",
	"min",
	"normalize",
	"texture",
	"sin",
	"tan",
	"transpose",
	"SetMeshOutputsEXT",
	"EmitMeshTasksEXT",
};

struct GLSLEmitter {
	// TODO: should instead store the blocks for each method...
	const SharedBlockReference &sbr;

	std::map <const Instruction *const, uint32_t> ids;

	std::string new_id(const Reference &ref) {
		auto ptr = ref.get();
		ids.emplace(ptr, ids.size());
		return std::format("lvar{}", ids.at(ptr));
	}

	std::string result;
	int32_t indentation;

	void emit_line(const std::string &line) {
		std::string space(4 * indentation, ' ');
		result += space + line + "\n";
	}

	#define emit_fmt_line(...) emit_line(std::format(__VA_ARGS__))
	
	void emit_newline() {
		result += "\n";
	}
};

//
// std::string expression(GLSLContext &ctx, Reference expr)
// {
// 	auto &value = *expr;
// 	vswitch (value) {
// 	vcase(FieldAccess): {
// 		auto &access = value.as <FieldAccess> ();
// 		return fmt::format("{}.f{}", expression(ctx, access.value), access.fidx);
// 	}
// 	vcase(ArrayAccess): {
// 		auto &access = value.as <ArrayAccess> ();
// 		if (access.value && access.value->is <GlobalIntrinsic> ()) {
// 			auto gi = access.value->as <GlobalIntrinsic> ();
// 			if (gi == GlobalIntrinsic::eMeshVertices) {
// 				return fmt::format("gl_MeshVerticesEXT[{}].gl_Position",
// 					expression(ctx, access.index));
// 			}
// 		}
// 		return fmt::format("{}[{}]",
// 			expression(ctx, access.value), expression(ctx, access.index));
// 	}
// 	vcase(Swizzle): {
// 		auto &swizzle = value.as <Swizzle> ();
// 		return fmt::format("{}.{}", expression(ctx, swizzle.value), repr(swizzle.code));
// 	}
// 	vcase(GlobalResource):
// 		return reference(ctx, value.as <GlobalResource> ());
// 	vcase(GlobalIntrinsic):
// 		return reference(ctx, value.as <GlobalIntrinsic> ());
// 	vcase(ThreadOutput):
// 		return reference(ctx, value.as <ThreadOutput> ());
// 	vcase(Argument):
// 		return reference(ctx, value.as <Argument> ());
// 	vcase(Invocation): {
// 		auto &invocation = value.as <Invocation> ();
// 		const Block *callee = invocation.sbr.get();
// 		std::string name = subroutine_name(ctx, callee);
//
// 		std::string out = name + "(";
// 		auto nargs = invocation.args.size();
// 		for (size_t i = 0; i < nargs; i++) {
// 			out += expression(ctx, invocation.args[i]);
// 			if (i + 1 < nargs)
// 				out += ", ";
// 		}
//
// 		return out + ")";
// 	}
// 	default:
// 		break;
// 	}
//
// 	std::println(std::cerr, "expression not implemented for {}", expr->repr());
// 	std::abort();
// 	return "?";
// }
//
// void statement(GLSLContext &ctx, Reference instruction)
// {
// 	auto &value = *instruction;
// 	vswitch (value) {
// 	vcase(Local): {
// 		auto name = reference(ctx, instruction);
// 		auto decl = type_repr(ctx, value.as <Local> ().type);
// 		ctx.result += fmt::format("{}{} {}{};\n",
// 			ctx.indent, decl.base, name, decl.suffix);
// 		return;
// 	}
// 	vcase(Store): {
// 		auto &store = value.as <Store> ();
// 		ctx.result += fmt::format("{}{} = {};\n",
// 			ctx.indent,
// 			reference(ctx, store.destination),
// 			expression(ctx, store.source));
// 		return;
// 	}
// 	vcase(BuiltinIntrinsic):
// 		ctx.result += fmt::format("{}{};\n",
// 			ctx.indent,
// 			expression(ctx, instruction));
// 		return;
// 	vcase(Invocation):
// 		ctx.result += fmt::format("{}{};\n",
// 			ctx.indent,
// 			expression(ctx, instruction));
// 		return;
// 	vcase(Branch): {
// 		auto &branch = value.as <Branch> ();
// 		auto emit_body = [&](const SharedBlockReference &blk) {
// 			auto prev = ctx.indent;
// 			ctx.indent += "    ";
// 			for (auto &nested : *blk) {
// 				if (nested->is <Store> () || nested->is <Invocation> ()
// 					|| nested->is <BuiltinIntrinsic> ()
// 					|| nested->is <Branch> () || nested->is <Loop> ()
// 					|| nested->is <Local> ())
// 					statement(ctx, nested);
// 			}
// 			ctx.indent = prev;
// 		};
//
// 		for (size_t i = 0; i < branch.segments.size(); i++) {
// 			auto &segment = branch.segments[i];
// 			auto kw = (i == 0) ? "if" : "else if";
// 			ctx.result += fmt::format("{}{} ({})\n",
// 				ctx.indent, kw, expression(ctx, segment.cond));
// 			ctx.result += fmt::format("{}{{\n", ctx.indent);
// 			emit_body(segment.body);
// 			ctx.result += fmt::format("{}}}\n", ctx.indent);
// 		}
//
// 		if (branch.fallback.has_value()) {
// 			ctx.result += fmt::format("{}else\n", ctx.indent);
// 			ctx.result += fmt::format("{}{{\n", ctx.indent);
// 			emit_body(branch.fallback.value());
// 			ctx.result += fmt::format("{}}}\n", ctx.indent);
// 		}
// 		return;
// 	}
// 	vcase(Loop): {
// 		auto &loop = value.as <Loop> ();
// 		auto emit_body = [&](const SharedBlockReference &blk) {
// 			auto prev = ctx.indent;
// 			ctx.indent += "    ";
// 			for (auto &nested : *blk) {
// 				if (nested->is <Store> () || nested->is <Invocation> ()
// 					|| nested->is <BuiltinIntrinsic> ()
// 					|| nested->is <Branch> () || nested->is <Loop> ()
// 					|| nested->is <Local> ())
// 					statement(ctx, nested);
// 			}
// 			ctx.indent = prev;
// 		};
//
// 		auto cond_expr = expression(ctx, loop.cond_value);
//
// 		if (loop.kind == LoopKind::eFor && loop.init.has_value())
// 			emit_body(loop.init.value());
//
// 		auto loop_kw = (loop.kind == LoopKind::eFor) ? "for (;;)" : "while (true)";
// 		ctx.result += fmt::format("{}{}\n", ctx.indent, loop_kw);
// 		ctx.result += fmt::format("{}{{\n", ctx.indent);
// 		emit_body(loop.cond);
// 		ctx.result += fmt::format("{}    if (!({}))\n", ctx.indent, cond_expr);
// 		ctx.result += fmt::format("{}        break;\n", ctx.indent);
// 		emit_body(loop.body);
// 		if (loop.kind == LoopKind::eFor && loop.step.has_value())
// 			emit_body(loop.step.value());
// 		ctx.result += fmt::format("{}}}\n", ctx.indent);
// 		return;
// 	}
// 	default:
// 		break;
// 	}
//
// 	ctx.result += fmt::format("{}?\n", ctx.indent);
// }
//
// void emit_preamble(GLSLContext &ctx)
// {
// 	ctx.result += "#version 460\n\n";
// 	ctx.result += "#extension GL_EXT_scalar_block_layout : require\n";
//
// 	auto model = ctx.block.context.model;
// 	if (model == ShaderStage::eTask or model == ShaderStage::eMesh) {
// 		ctx.result += "#extension GL_EXT_mesh_shader : require\n";
// 		ctx.result += "\n";
// 	}
// 	
// 	if (model == ShaderStage::eCompute
// 			or model == ShaderStage::eTask
// 			or model == ShaderStage::eMesh) {
// 		auto size = ctx.block.context.workgroup_size.value_or(
// 			std::array <uint32_t, 3> { 1, 1, 1 }
// 		);
// 		ctx.result += fmt::format(
// 			"layout (local_size_x = {}, local_size_y = {}, local_size_z = {}) in;\n\n",
// 			size[0], size[1], size[2]
// 		);
// 	}
//
// 	if (model == ShaderStage::eMesh) {
// 		if (!ctx.block.context.mesh_max_vertices.has_value()
// 			|| !ctx.block.context.mesh_max_primitives.has_value()) {
// 			std::println(std::cerr, "mesh shader missing MeshletPayload size information");
// 			std::abort();
// 		}
// 		auto max_vertices = ctx.block.context.mesh_max_vertices.value();
// 		auto max_primitives = ctx.block.context.mesh_max_primitives.value();
// 		ctx.result += fmt::format(
// 			"layout (max_vertices = {}, max_primitives = {}) out;\n",
// 			max_vertices, max_primitives
// 		);
// 		ctx.result += "layout (triangles) out;\n\n";
// 	}
// }
//
// void emit_task_payload(GLSLContext &ctx)
// {
// 	if (!ctx.block.context.task_payload_type.has_value())
// 		return;
//
// 	auto decl = type_repr(ctx, ctx.block.context.task_payload_type.value());
// 	ctx.result += fmt::format("taskPayloadSharedEXT {} task_payload{};\n\n",
// 		decl.base, decl.suffix);
// }
//
// void collect_push_constant_indices(
// 	const std::vector <const Block *> &blocks,
// 	std::map <void *, uint32_t> &pc_indices
// )
// {
// 	uint32_t pcounter = 0;
// 	for (auto *blk : blocks) {
// 		for (auto &[addr, refs] : blk->context.global_resources) {
// 			for (auto &ref : refs) {
// 				auto &grsrc = ref->as <GlobalResource> ();
// 				if (grsrc.kind != GlobalResourceKind::ePushConstant)
// 					continue;
//
// 				auto it = pc_indices.find(addr);
// 				if (it == pc_indices.end())
// 					it = pc_indices.emplace(addr, pcounter++).first;
// 				grsrc.push_constant_index = it->second;
// 			}
// 		}
// 	}
// }
//
// std::string resource_key(const GlobalResource &grsrc)
// {
// 	if (grsrc.kind == GlobalResourceKind::eSampler) {
// 		auto group = grsrc.group.value_or(0);
// 		auto index = grsrc.index.value_or(0);
// 		return fmt::format("sampler:{}:{}", group, index);
// 	}
//
// 	if (grsrc.kind == GlobalResourceKind::ePushConstant) {
// 		auto idx = grsrc.push_constant_index.value_or(0);
// 		auto offset = grsrc.push_constant_offset.value_or(0);
// 		return fmt::format("pc:{}:{}:{}:{}",
// 			idx, offset, (int) grsrc.layout, (void *) grsrc.type.get());
// 	}
//
// 	auto group = grsrc.group.value_or(0);
// 	auto index = grsrc.index.value_or(0);
// 	return fmt::format("buf:{}:{}:{}:{}:{}",
// 		(int) grsrc.kind, group, index, (int) grsrc.layout, (int) grsrc.access);
// }
//
// std::string resource_instance_name(const GlobalResource &grsrc)
// {
// 	return resource_base_name(grsrc);
// }
//
// void emit_buffer_fields(GLSLContext &ctx, const AggregateType &agg)
// {
// 	for (size_t i = 0; i < agg.size(); i++) {
// 		if (contains_unsized_array(agg[i]) && (i + 1 < agg.size())) {
// 			std::println(std::cerr, "unsized array must be the last field in a buffer block");
// 			std::abort();
// 		}
// 		auto decl = type_repr(ctx, agg[i]);
// 		ctx.result += fmt::format("    {} f{}{};\n", decl.base, i, decl.suffix);
// 	}
// }
//
// void emit_resource_decl(GLSLContext &ctx, GlobalResource &grsrc)
// {
// 	if (grsrc.kind == GlobalResourceKind::eSampler) {
// 		auto group = grsrc.group.value_or(0);
// 		auto index = grsrc.index.value_or(0);
// 		ctx.result += fmt::format("layout (set = {}, binding = {}) uniform sampler2D {};\n\n",
// 			group, index, resource_base_name(grsrc));
// 		return;
// 	}
//
// 	if (grsrc.kind == GlobalResourceKind::ePushConstant) {
// 		auto idx = grsrc.push_constant_index.value_or(0);
// 		grsrc.push_constant_index = idx;
// 		auto offset = grsrc.push_constant_offset.value_or(0);
// 		auto decl = type_repr(ctx, grsrc.type);
//
// 		ctx.result += fmt::format("layout ({}push_constant) uniform PC{} {{\n",
// 			layout_string(grsrc.layout), idx);
// 		ctx.result += fmt::format("    layout (offset = {}) {} pc{}{};\n",
// 			offset, decl.base, idx, decl.suffix);
// 		ctx.result += "};\n\n";
// 		return;
// 	}
//
// 	std::string modifier;
// 	switch (grsrc.kind) {
// 	case GlobalResourceKind::eUniformBuffer: modifier = "uniform"; break;
// 	case GlobalResourceKind::eStorageBuffer:
// 		switch (grsrc.access) {
// 		case GlobalResourceAccess::eRead: modifier = "readonly buffer"; break;
// 		case GlobalResourceAccess::eWrite: modifier = "writeonly buffer"; break;
// 		case GlobalResourceAccess::eReadWrite: modifier = "buffer"; break;
// 		default: modifier = "buffer"; break;
// 		}
// 		break;
// 	default:
// 		std::println(std::cerr, "unsupported global resource kind");
// 		std::abort();
// 	}
//
// 	auto group = grsrc.group.value_or(0);
// 	auto index = grsrc.index.value_or(0);
// 	auto instance = resource_instance_name(grsrc);
// 	ctx.result += fmt::format("layout ({}set = {}, binding = {}) {} R{}{} {{\n",
// 		layout_string(grsrc.layout), group, index, modifier, group, index);
// 	if (!grsrc.type || !grsrc.type->is <Type> ()) {
// 		std::println(std::cerr, "invalid resource type");
// 		std::abort();
// 	}
//
// 	auto &type = grsrc.type->as <Type> ();
// 	if (type.is <AggregateType> ()) {
// 		emit_buffer_fields(ctx, type.as <AggregateType> ());
// 	} else {
// 		auto decl = type_repr(ctx, grsrc.type);
// 		ctx.result += fmt::format("    {} value{};\n", decl.base, decl.suffix);
// 	}
// 	ctx.result += fmt::format("}} {};\n\n", instance);
// }
//
// void emit_global_resources(GLSLContext &ctx)
// {
// 	std::vector <const Block *> blocks;
// 	collect_blocks(ctx, blocks);
//
// 	std::map <void *, uint32_t> pc_indices;
// 	collect_push_constant_indices(blocks, pc_indices);
//
// 	std::set <std::string> emitted;
// 	for (auto *blk : blocks) {
// 		for (auto &[_, refs] : blk->context.global_resources) {
// 			for (auto &ref : refs) {
// 				auto &grsrc = ref->as <GlobalResource> ();
// 				auto key = resource_key(grsrc);
// 				if (emitted.contains(key))
// 					continue;
// 				emitted.emplace(key);
// 				emit_resource_decl(ctx, grsrc);
// 			}
// 		}
// 	}
// }
//
// std::string subroutine_return_type(GLSLContext &ctx, const Block &blk, uint32_t &out_argi)
// {
// 	// TODO: should have proper returns (and proper parameters instead of thread index)
// 	out_argi = 0;
// 	if (blk.context.thread_outputs.empty())
// 		return "void";
// 	if (blk.context.thread_outputs.size() > 1) {
// 		std::println(std::cerr, "subroutine return with multiple outputs is not supported");
// 		std::abort();
// 	}
//
// 	auto &tout = blk.context.thread_outputs.front();
// 	out_argi = tout.argi;
//
// 	auto repr = type_repr(ctx, tout.type);
// 	return repr.base + repr.suffix;
// }
//
// void emit_subroutine_function(GLSLContext &ctx, const Block &blk, const std::string &name)
// {
// 	set_active_block(ctx, blk);
// 	ctx.argument_names.clear();
// 	ctx.local_thread_outputs.clear();
//
// 	for (auto &arg : blk.context.arguments)
// 		ctx.argument_names.push_back(fmt::format("arg{}", arg.argi));
//
// 	uint32_t ret_argi = 0;
// 	auto return_type = subroutine_return_type(ctx, blk, ret_argi);
//
// 	ctx.result += fmt::format("{} {}(", return_type, name);
// 	for (size_t i = 0; i < blk.context.arguments.size(); i++) {
// 		auto &arg = blk.context.arguments[i];
// 		auto decl = type_repr(ctx, arg.type);
// 		ctx.result += fmt::format("{} {}{}", decl.base, ctx.argument_names[arg.argi], decl.suffix);
// 		if (i + 1 < blk.context.arguments.size())
// 			ctx.result += ", ";
// 	}
// 	ctx.result += ")\n{\n";
//
// 	// TODO: should just be one set of returns...
// 	// for perf maybe even prefer out parameters for tuples
// 	for (auto &tout : blk.context.thread_outputs) {
// 		auto name = fmt::format("lout{}", tout.argi);
// 		ctx.local_thread_outputs.emplace(tout.argi, name);
// 		auto decl = type_repr(ctx, tout.type);
// 		ctx.result += fmt::format("    {} {}{};\n", decl.base, name, decl.suffix);
// 	}
//
// 	if (blk.context.thread_outputs.size())
// 		ctx.result += "\n";
//
// 	emit_block_statements(ctx, blk);
//
// 	if (return_type != "void") {
// 		auto name_it = ctx.local_thread_outputs.find(ret_argi);
// 		auto local_name = (name_it != ctx.local_thread_outputs.end())
// 			? name_it->second
// 			: fmt::format("lout{}", ret_argi);
// 		ctx.result += fmt::format("    return {};\n", local_name);
// 	}
//
// 	ctx.result += "}\n\n";
// 	set_active_block(ctx, ctx.block);
// }
//
// void emit_subroutine_functions(GLSLContext &ctx)
// {
// 	std::vector <const Block *> order;
// 	std::set <const Block *> visited;
//
// 	auto visit = [&](auto &&self, const Block &blk) -> void {
// 		for (auto &instr : blk) {
// 			if (instr->is <Invocation> ()) {
// 				auto &inv = instr->as <Invocation> ();
// 				const Block *callee = inv.sbr.get();
// 				if (visited.contains(callee))
// 					continue;
// 				visited.emplace(callee);
// 				self(self, *callee);
// 				order.push_back(callee);
// 			}
// 		}
// 	};
//
// 	visit(visit, ctx.block);
//
// 	for (auto *callee : order) {
// 		if (callee->context.model != ShaderStage::eSubroutine)
// 			continue;
//
// 		auto name = subroutine_name(ctx, callee);
//
// 		if (ctx.emitted_subroutines.contains(callee))
// 			continue;
//
// 		ctx.emitted_subroutines.emplace(callee);
// 		emit_subroutine_function(ctx, *callee, name);
// 	}
// }
//
// void emit_main_function(GLSLContext &ctx)
// {
// 	ctx.result += "void main()\n";
// 	ctx.result += "{\n";
//
// 	emit_block_statements(ctx, ctx.block);
//
// 	ctx.result += "}";
// }
//
// std::string generate(GLSLContext &ctx, size_t tabs)
// {
// 	if (ctx.block.context.model == ShaderStage::eSubroutine) {
// 		emit_aggregate_decls(ctx);
// 		auto name = subroutine_name(ctx, &ctx.block);
// 		emit_subroutine_function(ctx, ctx.block, name);
// 		return ctx.result;
// 	} else {
// 		emit_preamble(ctx);
// 		emit_aggregate_decls(ctx);
// 		emit_task_payload(ctx);
// 		emit_thread_inputs(ctx);
// 		emit_thread_outputs(ctx);
// 		emit_global_resources(ctx);
// 		emit_subroutine_functions(ctx);
// 		emit_main_function(ctx);
// 		return ctx.result;
// 	}
// }

// auto collect_blocks(const SharedBlockReference &sbr)
// {
// 	std::set <SharedBlockReference> visited;
//
// 	auto visit = [&](auto &&self, const SharedBlockReference &sbr) -> void {
// 		if (visited.contains(sbr))
// 			return;
//
// 		visited.emplace(sbr);
// 		for (auto &instr : *sbr) {
// 			vswitch (*instr) {
// 			vcase(Invocation): {
// 				auto &inv = instr->as <Invocation> ();
// 				self(self, inv.sbr);
// 			}
// 			vcase(Branch): {
// 				auto &branch = instr->as <Branch> ();
// 				for (auto &segment : branch.segments)
// 					self(self, segment.body);
// 				if (branch.fallback.has_value())
// 					self(self, *branch.fallback);
// 			}
// 			vcase(Loop): {
// 				auto &loop = instr->as <Loop> ();
// 				if (loop.init.has_value())
// 					self(self, *loop.init);
// 				self(self, loop.cond);
// 				if (loop.step.has_value())
// 					self(self, *loop.step);
// 				self(self, loop.body);
// 			}
// 			default:
// 				break;
// 			}
// 		}
// 	};
//
// 	visit(visit, sbr);
//
// 	return std::vector(visited.begin(), visited.end());
// }

bool is_unsized_type(const Type &type)
{
	vswitch (type) {
	vcase(ArrayType): {
		auto &array = type.as <ArrayType> ();
		auto &base = array.base->as <Type> ();
		return (array.size <= 0)
			? true
			: is_unsized_type(base);
	}
	vcase(AggregateType): {
		auto &agg = type.as <AggregateType> ();
		for (auto &field : agg) {
			auto &ftype = field->as <Type> ();
			if (is_unsized_type(ftype))
				return true;
		}

		break;
	}
	default:
		break;
	}

	return false;
}

std::string grsrc_name(const GlobalResource &grsrc)
{
	auto group = grsrc.group.value_or(0);
	auto index = grsrc.index.value_or(0);
	return fmt::format("r{}b{}", group, index);
}

std::string lval_repr(const GLSLEmitter &em, const Reference &ref)
{
	vswitch (*ref) {
	vcase(ThreadOutput): {
		auto &tout = ref->as <ThreadOutput> ();
		return std::format("lout{}", tout.argi);
	}
	vcase(GlobalIntrinsic): {
		auto gintr = ref->as <GlobalIntrinsic> ();
		return g_global_intrinsics.at(std::to_underlying(gintr));
	}
	vcase(GlobalResource): {
		auto &grsrc = ref->as <GlobalResource> ();
		if (grsrc.kind == GlobalResourceKind::ePushConstant)
			return "pc";

		auto base = grsrc_name(grsrc);
		if (grsrc.kind == GlobalResourceKind::eSampler)
			return base;

		auto &type = grsrc.type->as <Type> ();
		return type.is <AggregateType> ()
			? base
			: base + ".value";
	}
	default:
		break;
	}

	auto ptr = ref.get();
	if (not em.ids.contains(ptr))
		fatal("no lval id entry for {}", ref->repr());

	return std::format("lvar{}", em.ids.at(ptr));
}

struct TypeRepr {
	std::string base;
	std::string suffix;
};

std::string primitive_repr(const PrimitiveType &primitive)
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

TypeRepr type_repr(const GLSLEmitter &em, const Reference &ref)
{
	auto &type = ref->as <Type> ();
	vswitch (type) {
	vcase(PrimitiveType): {
		auto &pt = type.as <PrimitiveType> ();
		auto str = primitive_repr(pt);
		return { str, "" };
	}
	vcase(AggregateType): {
		auto &agg = type.as <AggregateType> ();
		return { agg.name, "" };
	}
	vcase(ArrayType): {
		auto &array = type.as <ArrayType> ();
		auto size = array.size > 0 ? std::format("[{}]", array.size) : "[]";
		auto repr = type_repr(em, array.base);
		return { repr.base, repr.suffix + size };
	}
	default:
		break;
	}

	fatal("unhandled case for type_repr: {}", ref->repr());
}

std::string expr_repr(const GLSLEmitter &em, const Reference &ref);

std::string opn_repr(const GLSLEmitter &em, const Operation &opn)
{
	auto s = g_operation_code.at(std::to_underlying(opn.code));
	if (opn.code == OperationCode::eLogicalNot
			or opn.code == OperationCode::eBitNot)
		return std::format("({}{})", s, expr_repr(em, opn.a));

	return std::format("({} {} {})",
		expr_repr(em, opn.a), s, expr_repr(em, opn.b));
}

std::string expr_repr(const GLSLEmitter &em, const Reference &ref)
{
	auto args = [&](const std::vector <Reference> &args) {
		std::string result;
		for (size_t i = 0; i < args.size(); i++) {
			result += expr_repr(em, args[i]);
			if (i + 1 < args.size())
				result += ", ";
		}

		return result;
	};

	vswitch (*ref) {
	vcase(Constant): {
		return ref->as <Constant> ().repr();
	}
	vcase(Construct): {
		auto &ctor = ref->as <Construct> ();
		auto repr = type_repr(em, ctor.type);
		return std::format("{}{}({})",
			repr.base, repr.suffix, args(ctor.args));
	}
	vcase(Local): {
		return lval_repr(em, ref);
	}
	vcase(ThreadInput): {
		auto &tin = ref->as <ThreadInput> ();
		return fmt::format("lin{}", tin.argi);
	}
	vcase(Operation): {
		auto &opn = ref->as <Operation> ();
		return opn_repr(em, opn);
	}
	vcase(FieldAccess): {
		auto &facc = ref->as <FieldAccess> ();
		return std::format("{}.f{}", expr_repr(em, facc.value), facc.fidx);
	}
	vcase(GlobalResource): {
		return lval_repr(em, ref);
	}
	vcase(BuiltinIntrinsic): {
		auto &bintr = ref->as <BuiltinIntrinsic> ();
		auto ftn = g_intrinsic_code.at(std::to_underlying(bintr.code));
		return std::format("{}({})", ftn, args(bintr.args));
	}
	vcase(Swizzle): {
		auto &swz = ref->as <Swizzle> ();
		return std::format("{}.{}", expr_repr(em, swz.value), repr(swz.code));
	}
	vcase(ArrayAccess): {
		auto &aacc = ref->as <ArrayAccess> ();
		return std::format("{}[{}]",
			expr_repr(em, aacc.value),
			expr_repr(em, aacc.index));
	}
	default:
		break;
	}

	fatal("unhandled case for expr_repr: {}", ref->repr());
}

void emit_body(GLSLEmitter &em, const SharedBlockReference &sbr);

void emit_statement(GLSLEmitter &em, const Reference &ref)
{
	vswitch (*ref) {
	vcase(Local): {
		auto &local = ref->as <Local> ();
		auto name = em.new_id(ref);
		auto repr = type_repr(em, local.type);
		return em.emit_fmt_line("{} {}{};", repr.base, name, repr.suffix);
	}
	vcase(Store): {
		auto &store = ref->as <Store> ();
		auto dst = lval_repr(em, store.destination);
		auto src = expr_repr(em, store.source);
		return em.emit_fmt_line("{} = {};", dst, src);
	}
	vcase(BuiltinIntrinsic): {
		return em.emit_line(expr_repr(em, ref) + ";");
	}
	vcase(Loop): {
		auto &loop = ref->as <Loop> ();

		if (loop.kind == LoopKind::eFor and loop.init)
			emit_body(em, *loop.init);

		em.emit_line("for (;;) {");
		em.indentation++;

		// TODO: loop.cond vs cond_value?? need to check out...
		em.emit_fmt_line("if (!({})) break;", expr_repr(em, loop.cond_value));
		emit_body(em, loop.body);
		
		if (loop.kind == LoopKind::eFor and loop.step)
			emit_body(em, *loop.step);
		
		em.indentation--;
		em.emit_line("}");

		return em.emit_newline();
	}
	default:
		break;
	}
	
	fatal("unhandled case for emit_statement: {}", ref->repr());
}

void emit_body(GLSLEmitter &em, const SharedBlockReference &sbr)
{
	// TODO: use heuristics to guage which instructions should be promoted
	// std::vector <Reference> statements;

	for (auto &instr : *sbr) {
		vswitch (*instr) {
		vcase(Branch):
		vcase(BuiltinIntrinsic):
			// TODO: skip those without side effects
		vcase(Invocation):
		vcase(Local):
		vcase(Loop):
		vcase(Store):
			emit_statement(em, instr);
			break;
		default:
			break;
		}
	}
}

void emit_main(GLSLEmitter &em)
{
	em.emit_line("void main()");
	em.emit_line("{");
	em.indentation++;

	emit_body(em, em.sbr);
	
	em.indentation--;
	em.emit_line("}");
}

auto collect_extensions()
{
	std::vector <std::string> extensions;
	extensions.emplace_back("GL_EXT_scalar_block_layout");

	return extensions;
}

void emit_stage_io(GLSLEmitter &em)
{
	auto &tins = em.sbr->thread_inputs;
	for (auto &tin : tins) {
		auto repr = type_repr(em, tin.type);
		em.emit_fmt_line("layout (location = {}) in {} lin{}{};",
			tin.argi, repr.base, tin.argi, repr.suffix);
	}

	if (tins.size())
		em.emit_newline();

	auto &touts = em.sbr->thread_outputs;
	for (auto &tout : touts) {
		auto repr = type_repr(em, tout.type);
		auto rate = g_rate_strings.at(std::to_underlying(tout.properties));
		em.emit_fmt_line("layout (location = {}) {} out {} lout{}{};",
			tout.argi, rate, repr.base, tout.argi, repr.suffix);
	}

	if (touts.size())
		em.emit_newline();
}

void emit_structs(GLSLEmitter &em)
{
	// TODO: refactor AggregateX -> StructX
	auto cmp = [](const AggregateType &a, const AggregateType &b) {
		return a.name < b.name;
	};

	// TODO: make this an iteration over all method blocks
	std::set <AggregateType, decltype(cmp)> structs;
	for (auto &instr : *em.sbr) {
		if (not instr->is <Type> ())
			continue;

		auto &type = instr->as <Type> ();
		if (not type.is <AggregateType> ())
			continue;

		if (is_unsized_type(type))
			continue;

		auto &agg = type.as <AggregateType> ();
		structs.insert(agg);
	}

	for (auto &agg : structs) {
		em.emit_fmt_line("struct {} {{", agg.name);
		em.indentation++;
		for (const auto &[i, f] : std::views::enumerate(agg)) {
			auto repr = type_repr(em, f);
			em.emit_fmt_line("{} f{}{};", repr.base, i, repr.suffix);
		}
		em.indentation--;
		em.emit_line("};");
		em.emit_newline();
	}
}

std::string buffer_access(const GlobalResource &grsrc)
{
	switch (grsrc.kind) {
	case GlobalResourceKind::eUniformBuffer: return "uniform";
	case GlobalResourceKind::eStorageBuffer:
		switch (grsrc.access) {
		case GlobalResourceAccess::eRead: return "readonly buffer";
		case GlobalResourceAccess::eWrite: return "writeonly buffer";
		case GlobalResourceAccess::eReadWrite: return "buffer";
		default: return "buffer";
		}
		break;
	// TODO: buffer reference
	default:
		break;
	}

	fatal("unhandled case for buffer_access: {}", grsrc.repr());
}

void emit_resource(GLSLEmitter &em, const GlobalResource &grsrc)
{
	auto layout = g_resource_layouts.at(std::to_underlying(grsrc.layout));
	if (grsrc.kind == GlobalResourceKind::ePushConstant) {
		auto offset = grsrc.offset.value_or(0);
		auto repr = type_repr(em, grsrc.type);

		em.emit_fmt_line("layout ({}, push_constant) uniform PC {{", layout);
		em.indentation++;
		em.emit_fmt_line("layout (offset = {}) {} pc{};", offset, repr.base, repr.suffix);
		em.indentation--;
		em.emit_line("};");
		return em.emit_newline();
	}

	auto group = grsrc.group.value_or(0);
	auto index = grsrc.index.value_or(0);
	auto name = grsrc_name(grsrc);
	if (grsrc.kind == GlobalResourceKind::eSampler) {
		em.emit_fmt_line(
			"layout (set = {}, binding = {}) "
			"uniform sampler2D {};",
			group, index, name
		);
		return em.emit_newline();
	}

	// Rest are buffer types
	auto access = buffer_access(grsrc);

	em.emit_fmt_line(
		"layout ({}, set = {}, binding = {}) "
		"{} Buffer{}x{} {{",
		layout, group, index,
		access, group, index
	);

	auto &type = grsrc.type->as <Type> ();

	em.indentation++;
	if (type.is <AggregateType> ()) {
		auto &agg = type.as <AggregateType> ();
		for (const auto &[i, f] : std::views::enumerate(agg)) {
			auto repr = type_repr(em, f);
			em.emit_fmt_line("{} f{}{};", repr.base, i, repr.suffix);
		}
	} else {
		auto repr = type_repr(em, grsrc.type);
		em.emit_fmt_line("{} value{};", repr.base, repr.suffix);
	}
	em.indentation--;

	em.emit_fmt_line("}} {};", name);
	return em.emit_newline();
}

void emit_whole(GLSLEmitter &em)
{
	// Preamble section
	em.emit_line("#version 460");
	em.emit_newline();

	auto extensions = collect_extensions();
	for (auto &ext : extensions)
		em.emit_fmt_line("#extension {} : require", ext);
	em.emit_newline();

	// Struct declarations
	emit_structs(em);

	// Layout inputs and outputs
	emit_stage_io(em);

	// Global shader resources
	// NOTE: Top-level is sufficient because of context inheritence
	for (auto &[_, ref] : em.sbr->global_resources) {
		auto &grsrc = ref->as <GlobalResource> ();
		emit_resource(em, grsrc);
	}

	// Main method
	emit_main(em);
}

std::string generate_glsl(const SharedBlockReference &sbr)
{
	TSCOPE("generating glsl code");

	auto em = GLSLEmitter {
		.sbr = sbr,
		.result = "",
		.indentation = 0,
	};

	// TODO: do the preprocessing here ONCE then run the emitter in full...

	emit_whole(em);

	return em.result;
}

} // namespace rcgp
