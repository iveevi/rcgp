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
	"gl_VertexIndex",
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

static auto g_primitive_types = std::array {
	"bool",
	"int",
	"uint",
	"float",
	"uvec2",
	"uvec3",
	"uvec4",
	"ivec2",
	"ivec3",
	"ivec4",
	"vec2",
	"vec3",
	"vec4",
	"imat2",
	"imat2x3",
	"imat2x4",
	"imat3x2",
	"imat3",
	"imat3x4",
	"imat4x2",
	"imat4x3",
	"imat4",
	"umat2",
	"umat2x3",
	"umat2x4",
	"umat3x2",
	"umat3",
	"umat3x4",
	"umat4x2",
	"umat4x3",
	"umat4",
	"mat2",
	"mat2x3",
	"mat2x4",
	"mat3x2",
	"mat3",
	"mat3x4",
	"mat4x2",
	"mat4x3",
	"mat4",
};

struct GLSLEmitter {
	const SharedBlockReference &main;
	std::set <SharedBlockReference> subroutines;
	std::map <const Instruction *const, uint32_t> ids;
	std::string result;
	int32_t indentation;

	std::string new_id(const Reference &ref) {
		auto ptr = ref.get();
		ids.emplace(ptr, ids.size());
		return std::format("lvar{}", ids.at(ptr));
	}

	void emit_line(const std::string &line) {
		std::string space(4 * indentation, ' ');
		result += space + line + "\n";
	}

	#define emit_fmt_line(...) emit_line(std::format(__VA_ARGS__))
	
	void emit_newline() {
		result += "\n";
	}
};

bool is_unsized_type(const Type &type)
{
	vswitch (type) {
	vcase(Array): {
		auto &array = type.as <Array> ();
		auto &base = array.base->as <Type> ();
		return (array.size <= 0)
			? true
			: is_unsized_type(base);
	}
	vcase(Struct): {
		auto &agg = type.as <Struct> ();
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

std::string expr_repr(const GLSLEmitter &em, const Reference &ref);

std::string lval_repr(const GLSLEmitter &em, const Reference &ref)
{
	vswitch (*ref) {
	vcase(StageOutput): {
		auto &sout = ref->as <StageOutput> ();
		return std::format("lout{}", sout.argi);
	}
	vcase(Return): {
		auto &ret = ref->as <Return> ();
		return std::format("ret{}", ret.argi);
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
		return type.is <Struct> ()
			? base
			: base + ".value";
	}
	vcase(FieldAccess): {
		auto &facc = ref->as <FieldAccess> ();
		return std::format("{}.f{}", expr_repr(em, facc.value), facc.fidx);
	}
	vcase(ArrayAccess): {
		auto &aacc = ref->as <ArrayAccess> ();
		if (aacc.value->is <GlobalIntrinsic> ()) {
			auto &gintr = aacc.value->as <GlobalIntrinsic> ();
			if (gintr == GlobalIntrinsic::eMeshVertices) {
				return std::format(
					"gl_MeshVerticesEXT[{}].gl_Position",
					expr_repr(em, aacc.index)
				);
			}
		}

		return std::format("{}[{}]",
			lval_repr(em, aacc.value),
			expr_repr(em, aacc.index));
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

std::string primitive_repr(const Primitive &primitive)
{
	auto raw = std::to_underlying(primitive);
	if (raw < 0 || static_cast<size_t>(raw) >= g_primitive_types.size())
		fatal("unhandled primitive type string case");
	return g_primitive_types.at(static_cast<size_t>(raw));
}

TypeRepr type_repr(const GLSLEmitter &em, const Reference &ref)
{
	auto &type = ref->as <Type> ();
	vswitch (type) {
	vcase(Primitive): {
		auto &pt = type.as <Primitive> ();
		auto str = primitive_repr(pt);
		return { str, "" };
	}
	vcase(Struct): {
		auto &agg = type.as <Struct> ();
		return { agg.name, "" };
	}
	vcase(Array): {
		auto &array = type.as <Array> ();
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
	vcase(StageInput): {
		auto &sin = ref->as <StageInput> ();
		return fmt::format("lin{}", sin.argi);
	}
	vcase(Argument): {
		auto &arg = ref->as <Argument> ();
		return fmt::format("arg{}", arg.argi);
	}
	vcase(Operation): {
		auto &opn = ref->as <Operation> ();
		return opn_repr(em, opn);
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
	vcase(ArrayAccess):
	vcase(FieldAccess):
	vcase(GlobalResource):
	vcase(GlobalIntrinsic): {
		return lval_repr(em, ref);
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

		emit_body(em, loop.cond);
		em.emit_fmt_line("if (!({})) break;", expr_repr(em, loop.cond_value));
		emit_body(em, loop.body);
		
		if (loop.kind == LoopKind::eFor and loop.step)
			emit_body(em, *loop.step);
		
		em.indentation--;
		return em.emit_line("}");
	}
	vcase(Branch): {
		auto &branch = ref->as <Branch> ();

		for (const auto &[i, b] : std::views::enumerate(branch.segments)) {
			auto kw = (i == 0) ? "if" : "else if";
			em.emit_fmt_line("{} ({}) {{", kw, expr_repr(em, b.cond));
			em.indentation++;
			emit_body(em, b.body);
			em.indentation--;
			em.emit_line("}");
		}

		if (branch.fallback) {
			em.emit_line("else");
			em.indentation++;
			emit_body(em, branch.fallback.value());
			em.indentation--;
		}

		return;
	}
	vcase(Invocation): {
		auto &inv = ref->as <Invocation> ();
		
		std::string decl = inv.sbr->name + "(";
		for (const auto &[i, arg] : std::views::enumerate(inv.args)) {
			decl += expr_repr(em, arg);
			if (i + 1 < inv.args.size())
				decl += ", ";
		}

		if (inv.args.size() and inv.returns.size())
			decl += ", ";

		for (const auto &[i, ret] : std::views::enumerate(inv.returns)) {
			decl += lval_repr(em, ret);
			if (i + 1 < inv.returns.size())
				decl += ", ";
		}

		decl += ");";

		return em.emit_line(decl);
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

void emit_subroutine(GLSLEmitter &em, const SharedBlockReference &sbr)
{
	std::string args = "";

	for (const auto &[i, arg] : std::views::enumerate(sbr->arguments)) {
		auto repr = type_repr(em, arg.type);
		args += std::format("{} arg{}{}", repr.base, i, repr.suffix);
		if (i + 1 < sbr->returns.size())
			args += ", ";
	}

	if (sbr->arguments.size() and sbr->returns.size())
		args += ", ";

	for (const auto &[i, ret] : std::views::enumerate(sbr->returns)) {
		auto repr = type_repr(em, ret.type);
		args += std::format("out {} ret{}{}", repr.base, i, repr.suffix);
		if (i + 1 < sbr->returns.size())
			args += ", ";
	}

	em.emit_fmt_line("void {}({})", sbr->name, args);

	em.emit_line("{");
	em.indentation++;

	emit_body(em, em.main);
	
	em.indentation--;
	em.emit_line("}");
}

void emit_main(GLSLEmitter &em)
{
	em.emit_line("void main()");
	em.emit_line("{");
	em.indentation++;

	emit_body(em, em.main);
	
	em.indentation--;
	em.emit_line("}");
}

auto collect_extensions(const GLSLEmitter &em)
{
	std::vector <std::string> extensions;
	extensions.emplace_back("GL_EXT_scalar_block_layout");
	
	auto model = em.main->model;
	if (model == ShaderStage::eMesh or model == ShaderStage::eTask)
		extensions.emplace_back("GL_EXT_mesh_shader");

	return extensions;
}

void emit_stage_io(GLSLEmitter &em)
{
	auto &tins = em.main->stage_inputs;
	for (auto &tin : tins) {
		auto repr = type_repr(em, tin.type);
		em.emit_fmt_line("layout (location = {}) in {} lin{}{};",
			tin.argi, repr.base, tin.argi, repr.suffix);
	}

	if (tins.size())
		em.emit_newline();

	auto &touts = em.main->stage_outputs;
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
	auto cmp = [](const Struct &a, const Struct &b) {
		return a.name < b.name;
	};

	// TODO: make this an iteration over all method blocks
	std::set <Struct, decltype(cmp)> structs;
	for (auto &instr : *em.main) {
		if (not instr->is <Type> ())
			continue;

		auto &type = instr->as <Type> ();
		if (not type.is <Struct> ())
			continue;

		if (is_unsized_type(type))
			continue;

		auto &agg = type.as <Struct> ();
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
	if (type.is <Struct> ()) {
		auto &agg = type.as <Struct> ();
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

	auto extensions = collect_extensions(em);
	for (auto &ext : extensions)
		em.emit_fmt_line("#extension {} : require", ext);
	em.emit_newline();

	// Struct declarations
	emit_structs(em);

	// Layout inputs and outputs
	emit_stage_io(em);

	// Workgroup shape
	if (em.main->workgroup_size) {
		auto &shape = em.main->workgroup_size.value();
		em.emit_fmt_line(
			"layout ("
			"local_size_x = {}, "
			"local_size_y = {}, "
			"local_size_z = {}) in;",
			shape[0], shape[1], shape[2]
		);
		em.emit_newline();
	}

	// Mesh shader output
	if (em.main->model == ShaderStage::eMesh) {
		auto &max_vertices = em.main->mesh_max_vertices.value();
		auto &max_primitives = em.main->mesh_max_primitives.value();
		em.emit_fmt_line("layout ("
			"max_vertices = {}, "
			"max_primitives = {}) out;",
			max_vertices,
			max_primitives);
		em.emit_line("layout (triangles) out;");
		em.emit_newline();
	}

	// Task payload
	if (em.main->task_payload_type) {
		auto &payload = em.main->task_payload_type.value();
		auto repr = type_repr(em, payload);
		em.emit_fmt_line("taskPayloadSharedEXT {} task_payload{};",
			repr.base, repr.suffix);
		em.emit_newline();
	}

	// Global shader resources
	// NOTE: Top-level is sufficient because of context inheritence
	for (auto &[_, ref] : em.main->global_resources) {
		auto &grsrc = ref->as <GlobalResource> ();
		emit_resource(em, grsrc);
	}

	// Subroutines
	// TODO: top sort
	for (auto &sr : em.subroutines) {
		emit_subroutine(em, sr);
		em.emit_newline();
	}

	// Main method
	emit_main(em);
}

void collect_subroutines(GLSLEmitter &em, const SharedBlockReference &sbr)
{
	for (auto &inst : *sbr) {
		vswitch (*inst) {
		vcase(Invocation): {
			auto &inv = inst->as <Invocation> ();
			em.subroutines.insert(inv.sbr);
			collect_subroutines(em, inv.sbr);
			break;
		}
		vcase(Branch): {
			auto &branch = inst->as <Branch> ();
			for (auto &b : branch.segments)
				collect_subroutines(em, b.body);
			if (branch.fallback)
				collect_subroutines(em, branch.fallback.value());
			break;
		}
		vcase(Loop): {
			auto &loop = inst->as <Loop> ();
			if (loop.kind == LoopKind::eFor) {
				collect_subroutines(em, loop.init.value());
				collect_subroutines(em, loop.step.value());
			}

			collect_subroutines(em, loop.cond);
			collect_subroutines(em, loop.body);
			break;
		}
		default:
			break;
		}
	}
}

std::string generate_glsl(const SharedBlockReference &sbr)
{
	TSCOPE("generating glsl code");

	auto em = GLSLEmitter {
		.main = sbr,
		.result = "",
		.indentation = 0,
	};

	if (sbr->model == ShaderStage::eSubroutine) {
		emit_subroutine(em, sbr);
	} else {
		collect_subroutines(em, sbr);
		emit_whole(em);
	}

	return em.result;
}

} // namespace rcgp
