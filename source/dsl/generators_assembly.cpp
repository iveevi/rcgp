#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <print>
#include <set>
#include <source_location>
#include <vector>
#include <ranges>

#include <fmt/format.h>

#include "dsl/generators.hpp"
#include "dsl/instructions.hpp"
#include "util/error.hpp"

namespace rcgp {

struct AsmEmitter {
	std::map <void *, uint32_t> ids;
	std::string result;
	int32_t indentation;
	bool debug;

	void emit_line(const std::string &line) {
		std::string space(2 * indentation, ' ');
		result += space + line + "\n";
	}

	std::string ref(const Reference &ref) {
		// TODO: check for name in debug info
		auto ptr = ref.get();
		if (not ids.contains(ptr))
			ids.emplace(ptr, ids.size());

		return std::format("${}", ids.at(ptr));
	}
	
	std::string ref(const SharedBlockReference &sbr) {
		auto ptr = sbr.get();
		if (not ids.contains(ptr))
			ids.emplace(ptr, ids.size());

		return std::format("${}", ids.at(ptr));
	}
	
	void emit_assign(const Reference &dst, const std::string &value) {
		std::string space(2 * indentation, ' ');
		result += space + ref(dst) + " = " + value + "\n";
	}
	
	#define emit_fmt_line(...) emit_line(std::format(__VA_ARGS__))
	#define emit_fmt_assign(ref, ...) emit_assign(ref, std::format(__VA_ARGS__))
};

std::string strargs(AsmEmitter &em, const std::vector <Reference> &args)
{
	std::string result;
	for (const auto &[i, arg] : std::views::enumerate(args)) {
		result += em.ref(arg);
		if (i + 1 < args.size())
			result += ", ";
	}

	return result;
}

std::string emit_type(AsmEmitter &em, const Type &type)
{
	vswitch (type) {
	vcase(Primitive): {
		return std::string(repr(type.as <Primitive> ()));
	}
	vcase(Struct): {
		auto &st = type.as <Struct> ();
		// TODO: show true field names
		std::string result;
		for (const auto &[i, f] : std::views::enumerate(st)) {
			result += std::format("f{}: {}", i, em.ref(f));
			if (i + 1 < st.size())
				result += ", ";
		}
		return st.name + " { " + result + " }";
	}
	vcase(Array): {
		auto &array = type.as <Array> ();
		return std::format("{}[{}]",
			emit_type(em, array.base->as <Type> ()),
			(array.size > 0) ? std::format("{}", array.size) : ""
		);
	}
	default:
		break;
	}

	fatal("unhandled type in emit_type: {}", type.repr());
}

std::string emit_instr_value(AsmEmitter &em, const Reference &ref)
{
	vswitch (*ref) {
	vcase(Type): {
		return emit_type(em, ref->as <Type> ());
	}
	vcase(Constant): {
		return ref->as <Constant> ().repr();
	}
	vcase(Operation): {
		auto &opn = ref->as <Operation> ();
		auto base = repr(opn.code);
		if (opn.code == OperationCode::eLogicalNot
			or opn.code == OperationCode::eBitNot) {
			return std::format("{} {}", base, em.ref(opn.a));
		} else {
			return std::format(
				"{} {} {}",
				base, em.ref(opn.a), em.ref(opn.b)
			);
		}
	}
	vcase(Argument): {
		auto &arg = ref->as <Argument> ();
		return std::format("Argument {}: {}", arg.argi, em.ref(arg.type));
	}
	vcase(Construct): {
		auto &ctor = ref->as <Construct> ();
		return std::format("New {}({})", em.ref(ctor.type), strargs(em, ctor.args));
	}
	vcase(Local): {
		return em.ref(ref);
	}
	vcase(Swizzle): {
		auto &swz = ref->as <Swizzle> ();
		return std::format("Swizzle({}: {})", em.ref(swz.value), repr(swz.code));
	}
	vcase(FieldAccess): {
		auto &facc = ref->as <FieldAccess> ();
		// TODO: use true field names
		return std::format("{}.f{}", em.ref(facc.value), facc.fidx);
	}
	vcase(ArrayAccess): {
		auto &aacc = ref->as <ArrayAccess> ();
		return std::format("{}[{}]", em.ref(aacc.value), em.ref(aacc.index));
	}
	vcase(GlobalIntrinsic): {
		auto &gintr = ref->as <GlobalIntrinsic> ();
		return std::format("SV: {}", repr(gintr));
	}
	vcase(StageOutput): {
		auto &sout = ref->as <StageOutput> ();
		return std::format(
			"StageOutput {}: {} {}",
			sout.argi,
			repr(sout.properties),
			em.ref(sout.type)
		);
	}
	vcase(StageInput): {
		auto &sin = ref->as <StageInput> ();
		return std::format(
			"StageInput {}: {}",
			sin.argi,
			em.ref(sin.type)
		);
	}
	vcase(GlobalResource): {
		auto &grsrc = ref->as <GlobalResource> ();
		if (grsrc.kind == GlobalResourceKind::ePushConstant) {
			return std::format(
				"{} +{}: {} {}",
				repr(grsrc.kind),
				grsrc.offset.value_or(-1),
				repr(grsrc.layout),
				em.ref(grsrc.type)
			);
		}

		auto binding = std::format(
			"r{}b{}",
			grsrc.group.value_or(-1),
			grsrc.index.value_or(-1)
		);

		if (grsrc.kind == GlobalResourceKind::eSampler)
			return std::format("{} @{}", repr(grsrc.kind), binding);

		return std::format(
			"{} @{}: {} {}",
			repr(grsrc.kind),
			binding,
			repr(grsrc.layout),
			em.ref(grsrc.type)
		);
	}
	// vcase(BuiltinIntrinsic): {
	// 	auto &bintr = ref->as <BuiltinIntrinsic> ();
	default:
		break;
	};
	
	fatal("unhandled instruction in emit_instr_value: {}", ref->repr());
}

void emit_block(AsmEmitter &em, const SharedBlockReference &sbr);

void emit_instr(AsmEmitter &em, const Reference &ref)
{
	vswitch (*ref) {
	vcase(Argument):
	vcase(ArrayAccess):
	vcase(Constant):
	vcase(Construct):
	vcase(FieldAccess):
	vcase(GlobalIntrinsic):
	vcase(GlobalResource):
	vcase(Operation):
	vcase(StageInput):
	vcase(StageOutput):
	vcase(Swizzle):
	vcase(Type): {
		auto value = emit_instr_value(em, ref);
		return em.emit_assign(ref, value);
	}
	vcase(Local): {
		auto &local = ref->as <Local> ();
		return em.emit_fmt_assign(ref, "Local {}", em.ref(local.type));
	}
	vcase(Store): {
		auto &store = ref->as <Store> ();
		return em.emit_fmt_line("Store {} {}",
			em.ref(store.destination),
			em.ref(store.source)
		);
	}
	vcase(Loop): {
		auto &loop = ref->as <Loop> ();
		emit_block(em, loop.body);
		return em.emit_fmt_line("Loop {}", em.ref(loop.body));
	}
	vcase(Branch): {
		auto &branch = ref->as <Branch> ();
		for (auto &b : branch.segments)
			emit_block(em, b.body);
		if (branch.fallback)
			emit_block(em, branch.fallback.value());

		std::string result = "Branch ";
		for (const auto &[i, b] : std::views::enumerate(branch.segments)) {
			result += std::format("{}: {}", em.ref(b.cond), em.ref(b.body));
			if (i + 1 < branch.segments.size())
				result += ", ";
		}

		if (branch.fallback)
			result += std::format(", else: {}", em.ref(branch.fallback.value()));

		return em.emit_line(result);
	}
	vcase(BuiltinIntrinsic): {
		auto &bintr = ref->as <BuiltinIntrinsic> ();
		switch (bintr.code) {
		case BuiltinIntrinsicCode::eBreak:
			return em.emit_line("Break");
		case BuiltinIntrinsicCode::eContinue:
			return em.emit_line("Continue");
		case BuiltinIntrinsicCode::eDiscard:
			return em.emit_line("Discard");
		case BuiltinIntrinsicCode::eEmitMeshTasksEXT:
		case BuiltinIntrinsicCode::eSetMeshOutputsEXT:
			return em.emit_fmt_line("{}({})", repr(bintr.code), strargs(em, bintr.args));
		default:
			break;
		}

		return em.emit_fmt_assign(
			ref,
			"{}({})",
			repr(bintr.code),
			strargs(em, bintr.args)
		);
	}
	vcase(Return): {
		auto &ret = ref->as <Return> ();
		return em.emit_fmt_assign(ref, "Return {}: {}", ret.argi, em.ref(ret.type));
	}
	vcase(Invocation): {
		auto &inv = ref->as <Invocation> ();
		std::string result;
		result += inv.sbr->name + "(";
		result += strargs(em, inv.args);
		if (inv.args.size() and inv.returns.size())
			result += ", ";
		result += strargs(em, inv.returns) + ")";
		return em.emit_line(result);
	}
	default:
		break;
	}

	fatal("unhandled instruction in emit_instr: {}", ref->repr());
}

void emit_block(AsmEmitter &em, const SharedBlockReference &sbr)
{
	em.emit_fmt_line("{} = Block {{", em.ref(sbr));
	em.indentation++;
	
	for (auto &instr : *sbr)
		emit_instr(em, instr);

	em.indentation--;
	em.emit_line("}");
}

void emit_context(AsmEmitter &em, const SharedBlockReference &sbr)
{
	em.emit_line("Context {");
	em.indentation++;

	em.emit_fmt_line("stage: {},", repr(sbr->stage));

	if (sbr->stage == ShaderStage::eSubroutine)
		em.emit_fmt_line("name: {},", sbr->name);

	if (sbr->stage_inputs.size()) {
		std::string result;
		for (const auto &[i, sin] : std::views::enumerate(sbr->stage_inputs)) {
			result += std::format("{}", em.ref(sin.type));
			if (i + 1 < sbr->stage_inputs.size())
				result += ", ";
		}

		em.emit_fmt_line("stage inputs: {{ {} }}", result);
	}
	
	if (sbr->stage_outputs.size()) {
		std::string result;
		for (const auto &[i, sout] : std::views::enumerate(sbr->stage_outputs)) {
			result += std::format("{} {}", repr(sout.properties), em.ref(sout.type));
			if (i + 1 < sbr->stage_outputs.size())
				result += ", ";
		}

		em.emit_fmt_line("stage inputs: {{ {} }}", result);
	}

	if (sbr->global_resources.size()) {
		std::string result;

		size_t i = 0;
		for (const auto &[_, grsrc] : sbr->global_resources) {
			result += em.ref(grsrc);
			if (++i < sbr->global_resources.size())
				result += ", ";
		}

		em.emit_fmt_line("resources: {{ {} }},", result);
	}

	if (sbr->arguments.size()) {
		// TODO: shorthand which takes a lambda...
		std::string result;
		for (const auto &[i, arg] : std::views::enumerate(sbr->arguments)) {
			result += em.ref(arg.type);
			if (i + 1 < sbr->arguments.size())
				result += ", ";
		}

		em.emit_fmt_line("arguments: {{ {} }},", result);
	}
	
	if (sbr->returns.size()) {
		std::string result;
		for (const auto &[i, ret] : std::views::enumerate(sbr->returns)) {
			result += em.ref(ret.type);
			if (i + 1 < sbr->returns.size())
				result += ", ";
		}

		em.emit_fmt_line("returns: {{ {} }},", result);
	}
	
	em.indentation--;
	em.emit_line("}");
}

void emit_primary_block(AsmEmitter &em, const SharedBlockReference &sbr)
{
	em.emit_line("Block {");
	em.indentation++;
	
	emit_context(em, sbr);
	
	for (auto &instr : *sbr)
		emit_instr(em, instr);

	em.indentation--;
	em.emit_line("}");
}

std::string generate_assembly(const SharedBlockReference &sbr, bool debug)
{
	// TODO: option to show types directly (i.e. vec2, instead of $2)
	// also inlining constants...
	// bool verbose = false -> disregards types and such
	auto em = AsmEmitter {
		.ids = {},
		.result = "",
		.indentation = 0,
		.debug = debug,
	};

	emit_primary_block(em, sbr);
	
	return em.result;
}

} // namespace rcgp
