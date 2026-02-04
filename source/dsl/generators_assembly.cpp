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

std::string emit_instr_value(AsmEmitter &em, const Reference &ref);

std::string strargs(AsmEmitter &em, const std::vector <Reference> &args)
{
	std::string result;
	for (const auto &[i, arg] : std::views::enumerate(args)) {
		result += emit_instr_value(em, arg);
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
	default:
		break;
	};
	
	fatal("unhandled instruction in emit_instr_value: {}", ref->repr());
}

void emit_block(AsmEmitter &em, const SharedBlockReference &sbr);

void emit_instr(AsmEmitter &em, const Reference &ref)
{
	vswitch (*ref) {
	vcase(Constant):
	vcase(Operation):
	vcase(Swizzle):
	vcase(Argument):
	vcase(Construct):
	vcase(FieldAccess):
	vcase(ArrayAccess):
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
		default:
			break;
		}

		return em.emit_fmt_line("{}({})", repr(bintr.code), strargs(em, bintr.args));
	}
	vcase(Return): {
		auto &ret = ref->as <Return> ();
		return em.emit_fmt_assign(ref, "Return {}: {}", ret.argi, em.ref(ret.type));
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
	// TODO: more stuff
	
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
