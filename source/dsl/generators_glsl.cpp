#include <set>
#include <queue>
#include <cctype>

#include <fmt/format.h>

#include "dsl/generators.hpp"
#include "dsl/block.hpp"
#include "dsl/instructions.hpp"
#include "util/error.hpp"

namespace rcgp {

struct GLSLEmitter {
	const SharedBlockReference &main;
	std::vector <SharedBlockReference> subroutines;
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

std::string sanitize_name(const std::string &name)
{
	std::string sanitized;
	sanitized.reserve(name.size());

	bool prev_underscore = false;
	auto push_underscore = [&]() {
		if (sanitized.empty() or prev_underscore)
			return;

		sanitized.push_back('_');
		prev_underscore = true;
	};

	for (char ch : name) {
		auto uch = static_cast <unsigned char> (ch);
		if (std::isalnum(uch) or ch == '_') {
			sanitized.push_back(ch);
			prev_underscore = false;
			continue;
		}

		push_underscore();
	}

	while (not sanitized.empty() and sanitized.back() == '_')
		sanitized.pop_back();

	return sanitized;
}

struct TypeRepr {
	std::string base;
	std::string suffix;
};

TypeRepr type_repr(const GLSLEmitter &em, const Reference &ref)
{
	auto &type = ref->as <Type> ();
	vswitch (type) {
	vcase(Primitive): {
		auto &pt = type.as <Primitive> ();
		auto str = repr_glsl(pt);
		return { str, "" };
	}
	vcase(Struct): {
		auto &st = type.as <Struct> ();
		return { sanitize_name(st.name), "" };
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
	vcase(SystemValue): {
		auto sysval = ref->as <SystemValue> ();
		return repr_glsl(sysval);
	}
	vcase(GlobalResource): {
		auto &grsrc = ref->as <GlobalResource> ();
		
		auto base = grsrc_name(grsrc);
		switch (grsrc.kind) {
		case GlobalResourceKind::ePushConstant:
			return "pc";
		case GlobalResourceKind::eSampler:
		case GlobalResourceKind::eStorageImage:
		case GlobalResourceKind::eAccelerationStructure:
			return base;
		case GlobalResourceKind::eRayReceiverPayload:
		case GlobalResourceKind::eRayDispatcherPayload:
			return std::format("payload{}", grsrc.index.value_or(0));
		default:
			break;
		}

		auto &type = grsrc.type->as <Type> ();
		return type.is <Struct> ()
			? base
			: base + ".value";
	}
	vcase(FieldAccess): {
		auto &facc = ref->as <FieldAccess> ();
		auto &st = get_struct(em.main, facc.value);
		return std::format("{}.{}", expr_repr(em, facc.value), st.fields[facc.fidx]);
	}
	vcase(ArrayAccess): {
		auto &aacc = ref->as <ArrayAccess> ();
		if (aacc.value->is <SystemValue> ()) {
			auto &sysval = aacc.value->as <SystemValue> ();
			if (sysval == SystemValue::eMeshVertices) {
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

std::string expr_repr(const GLSLEmitter &em, const Reference &ref);

std::string opn_repr(const GLSLEmitter &em, const Operation &opn)
{
	auto s = repr_glsl(opn.code);
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
		// TODO: separate method
		auto &bintr = ref->as <BuiltinIntrinsic> ();
		switch (bintr.code) {
		case BuiltinIntrinsicCode::eBreak: return "break";
		case BuiltinIntrinsicCode::eContinue: return "continue";
		case BuiltinIntrinsicCode::eDiscard: return "discard";
		case BuiltinIntrinsicCode::eSelect:
			return std::format(
				"({} ? {} : {})",
				expr_repr(em, bintr.args.at(0)),
				expr_repr(em, bintr.args.at(1)),
				expr_repr(em, bintr.args.at(2))
			);
		case BuiltinIntrinsicCode::eUnsizedArrayLength:
			return std::format("{}.length()", expr_repr(em, bintr.args.at(0)));
		default:
			break;
		}
		auto ftn = repr_glsl(bintr.code);
		return std::format("{}({})", ftn, args(bintr.args));
	}
	vcase(Swizzle): {
		auto &swz = ref->as <Swizzle> ();
		return std::format("{}.{}", expr_repr(em, swz.value), repr(swz.code));
	}
	vcase(ArrayAccess):
	vcase(FieldAccess):
	vcase(GlobalResource):
	vcase(SystemValue): {
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
		if (local.init) {
			return em.emit_fmt_line(
				"{} {}{} = {};",
				repr.base, name,
				repr.suffix, expr_repr(em, local.init)
			);
		}
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

		em.emit_line("while (true) {");
		em.indentation++;

		emit_body(em, loop.body);
		
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
			em.emit_line("else {");
			em.indentation++;
			emit_body(em, branch.fallback.value());
			em.indentation--;
			em.emit_line("}");
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

bool has_side_effects(const BuiltinIntrinsicCode &code)
{
	static const std::set <BuiltinIntrinsicCode> side_effects {
		BuiltinIntrinsicCode::eBreak,
		BuiltinIntrinsicCode::eContinue,
		BuiltinIntrinsicCode::eDiscard,
		BuiltinIntrinsicCode::eEmitMeshTasksEXT,
		BuiltinIntrinsicCode::eImageStore,
		BuiltinIntrinsicCode::eSetMeshOutputsEXT,
		BuiltinIntrinsicCode::eTraceRays,
	};

	return side_effects.contains(code);
}

void emit_body(GLSLEmitter &em, const SharedBlockReference &sbr)
{
	// TODO: use heuristics to guage which instructions should be promoted
	// base it on # of instructions and # of uses
	// std::vector/set <Reference> statements;

	for (auto &instr : *sbr) {
		vswitch (*instr) {
		vcase(BuiltinIntrinsic): {
			auto &bintr = instr->as <BuiltinIntrinsic> ();
			if (not has_side_effects(bintr.code))
				break;
		}
		vcase(Branch):
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
		if (i + 1 < sbr->arguments.size())
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

	emit_body(em, sbr);
	
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
	
	auto stage = em.main->stage;
	if (stage == ShaderStage::eMesh
		or stage == ShaderStage::eTask)
		extensions.emplace_back("GL_EXT_mesh_shader");
	if (stage == ShaderStage::eRayGeneration
		or stage == ShaderStage::eClosestHit
		or stage == ShaderStage::eMiss)
		extensions.emplace_back("GL_EXT_ray_tracing");

	return extensions;
}

void emit_stage_io(GLSLEmitter &em)
{
	auto &tins = em.main->stage_inputs;
	for (auto &tin : tins) {
		auto repr = type_repr(em, tin.type);
		if (em.main->stage == ShaderStage::eFragment) {
			auto rate = repr_glsl(tin.properties);
			em.emit_fmt_line("layout (location = {}) {} in {} lin{}{};",
				tin.argi, rate, repr.base, tin.argi, repr.suffix);
		} else {
			em.emit_fmt_line("layout (location = {}) in {} lin{}{};",
				tin.argi, repr.base, tin.argi, repr.suffix);
		}
	}

	if (tins.size())
		em.emit_newline();

	auto &touts = em.main->stage_outputs;
	for (auto &tout : touts) {
		auto repr = type_repr(em, tout.type);
		if (em.main->stage == ShaderStage::eVertex
			or em.main->stage == ShaderStage::eMesh) {
			auto rate = repr_glsl(tout.properties);
			em.emit_fmt_line("layout (location = {}) {} out {} lout{}{};",
				tout.argi, rate, repr.base, tout.argi, repr.suffix);
		} else {
			em.emit_fmt_line("layout (location = {}) out {} lout{}{};",
				tout.argi, repr.base, tout.argi, repr.suffix);
		}
	}

	if (touts.size())
		em.emit_newline();
}

void collect_struct_dependencies(std::set <std::string> &deps, const Reference &ref)
{
	auto &type = ref->as <Type> ();
	vswitch (type) {
	vcase(Struct):
		deps.insert(type.as <Struct> ().name);
		break;
	vcase(Array):
		collect_struct_dependencies(deps, type.as <Array> ().base);
		break;
	default:
		break;
	}
}

auto top_sort_structs(const std::map <std::string, Struct> structs)
{
	std::map <std::string, std::set <std::string>> dependents;
	std::map <std::string, uint32_t> indegrees;
	for (auto &[name, _] : structs) {
		dependents[name] = {};
		indegrees[name] = 0;
	}

	for (auto &[name, st] : structs) {
		std::set <std::string> deps;
		for (auto &field : st)
			collect_struct_dependencies(deps, field);

		deps.erase(name);
		for (auto &dep : deps) {
			if (not structs.contains(dep))
				continue;

			if (dependents[dep].insert(name).second)
				indegrees[name]++;
		}
	}

	std::set <std::string> ready;
	for (auto &[name, degree] : indegrees) {
		if (degree == 0)
			ready.insert(name);
	}

	std::vector <Struct> sorted;
	sorted.reserve(structs.size());

	while (ready.size()) {
		auto name = *ready.begin();
		ready.erase(ready.begin());
		sorted.push_back(structs.at(name));

		for (auto &dependent : dependents.at(name)) {
			auto &degree = indegrees.at(dependent);
			degree--;
			if (degree == 0)
				ready.insert(dependent);
		}
	}

	if (sorted.size() != structs.size())
		fatal("cyclic struct dependency while emitting GLSL structs");

	return sorted;
}

void emit_structs(GLSLEmitter &em)
{
	// TODO: make this an iteration over all method blocks
	std::map <std::string, Struct> structs;
	for (auto &instr : *em.main) {
		if (not instr->is <Type> ())
			continue;

		auto &type = instr->as <Type> ();
		if (not type.is <Struct> ())
			continue;

		if (is_unsized_type(type))
			continue;

		auto &agg = type.as <Struct> ();
		if (not structs.contains(agg.name))
			structs.emplace(agg.name, agg);
	}

	auto sorted = top_sort_structs(structs);
	for (auto &st : sorted) {
		em.emit_fmt_line("struct {} {{", sanitize_name(st.name));
		em.indentation++;
		for (const auto &[i, f] : std::views::enumerate(st)) {
			auto repr = type_repr(em, f);
			em.emit_fmt_line("{} {}{};", repr.base, st.fields[i], repr.suffix);
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
	auto layout = repr_glsl(grsrc.layout);

	// Push constants
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

	// Storage images
	if (grsrc.kind == GlobalResourceKind::eStorageImage) {
		// TODO: method
		std::string access = " ";
		if (grsrc.access == GlobalResourceAccess::eRead)
			access = "readonly ";
		else if (grsrc.access == GlobalResourceAccess::eWrite)
			access = "writeonly ";

		em.emit_fmt_line(
			"layout (set = {}, binding = {}) "
			"uniform {}image2D {};",
			group, index, access, name
		);
		return em.emit_newline();
	}

	// Combined image samplers
	if (grsrc.kind == GlobalResourceKind::eSampler) {
		// TODO: get dimension and prefix
		em.emit_fmt_line(
			"layout (set = {}, binding = {}) "
			"uniform sampler2D {};",
			group, index, name
		);
		return em.emit_newline();
	}

	// Acceleration structures
	if (grsrc.kind == GlobalResourceKind::eAccelerationStructure) {
		em.emit_fmt_line(
			"layout (set = {}, binding = {}) "
			"uniform accelerationStructureEXT {};",
			group, index, name
		);
		return em.emit_newline();
	}

	// Ray payloads
	auto repr = type_repr(em, grsrc.type);
	if (grsrc.kind == GlobalResourceKind::eRayDispatcherPayload) {
		em.emit_fmt_line(
			"layout (location = {}) "
			"rayPayloadEXT {} payload{}{};",
			index, repr.base, index, repr.suffix
		);
		return em.emit_newline();
	} else if (grsrc.kind == GlobalResourceKind::eRayReceiverPayload) {
		em.emit_fmt_line(
			"layout (location = {}) "
			"rayPayloadInEXT {} payload{}{};",
			index, repr.base, index, repr.suffix
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
		auto &st = type.as <Struct> ();
		for (const auto &[i, f] : std::views::enumerate(st)) {
			auto repr = type_repr(em, f);
			em.emit_fmt_line("{} {}{};", repr.base, st.fields[i], repr.suffix);
		}
	} else {
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

	if (extensions.size())
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
	if (em.main->stage == ShaderStage::eMesh) {
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
	
	// Hit attribute
	if (em.main->hit_attribute_type) {
		auto &payload = em.main->hit_attribute_type.value();
		auto repr = type_repr(em, payload);
		em.emit_fmt_line("hitAttributeEXT {} hit_attribute{};",
			repr.base, repr.suffix);
		em.emit_newline();
	}

	// Global shader resources
	// NOTE: Top-level is sufficient because of context inheritence
	for (auto &[_, refs] : em.main->global_resources) {
		for (auto &ref : refs) {
			auto &grsrc = ref->as <GlobalResource> ();
			emit_resource(em, grsrc);
		}
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

using SbrMap = std::map <SharedBlockReference, std::set <SharedBlockReference>>;

void get_subroutines(SbrMap &call_to, SbrMap &call_from, const SharedBlockReference &sbr, const SharedBlockReference &whole)
{
	if (not call_to.contains(whole)) call_to[whole] = {};
	if (not call_from.contains(whole)) call_from[whole] = {};

	for (auto &inst : *sbr) {
		vswitch (*inst) {
		vcase(Invocation): {
			auto &inv = inst->as <Invocation> ();
			call_to[whole].insert(inv.sbr);
			call_from[inv.sbr].insert(whole);
			get_subroutines(call_to, call_from, inv.sbr, inv.sbr);
			break;
		}
		vcase(Branch): {
			auto &branch = inst->as <Branch> ();
			for (auto &b : branch.segments)
				get_subroutines(call_to, call_from, b.body, whole);
			if (branch.fallback)
				get_subroutines(call_to, call_from, branch.fallback.value(), whole);
			break;
		}
		vcase(Loop): {
			auto &loop = inst->as <Loop> ();
			get_subroutines(call_to, call_from, loop.body, whole);
			break;
		}
		default:
			break;
		}
	}
}

auto top_sort_sbrs(SbrMap &call_to, SbrMap &call_from)
{
	// to: calling to
	// from: invoked from
	std::vector <SharedBlockReference> sorted;
	sorted.reserve(call_to.size());

	std::queue <SharedBlockReference> queue;
	for (auto &[sbr, s] : call_to) {
		if (s.empty())
			queue.push(sbr);
	}

	while (queue.size()) {
		auto &sbr = queue.front();
		queue.pop();

		sorted.push_back(sbr);

		for (auto &callee : call_from[sbr]) {
			call_to[callee].erase(sbr);
			if (call_to[callee].empty())
				queue.push(callee);
		}
	}

	return sorted;
}

std::string generate_glsl(const SharedBlockReference &sbr)
{
	auto em = GLSLEmitter {
		.main = sbr,
		.result = "",
		.indentation = 0,
	};

	if (sbr->stage == ShaderStage::eSubroutine) {
		emit_subroutine(em, sbr);
	} else {
		// TODO: should happen at the
		// end of function construction
		// or at the end of a jems::scope...
		SbrMap call_to;
		SbrMap call_from;
		get_subroutines(call_to, call_from, sbr, sbr);

		if (call_to.size() > 1) {
			// Don't involve main in the sorting
			for (auto &[sbr, s] : call_from)
				s.erase(em.main);

			em.subroutines = top_sort_sbrs(call_to, call_from);
		}

		emit_whole(em);
	}

	return em.result;
}

} // namespace rcgp
