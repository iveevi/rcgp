#include <map>
#include <print>
#include <set>
#include <algorithm>
#include <string_view>
#include <type_traits>

#include "dsl/block.hpp"
#include "dsl/instructions.hpp"
#include "dsl/optimization.hpp"
#include "util/error.hpp"

namespace rcgp {

struct InstructionSet {
	std::set <Reference> accounted;
	std::vector <Reference> ordered;

	void add(const Reference &ref) {
		if (not accounted.contains(ref)) {
			accounted.insert(ref);
			ordered.emplace_back(ref);
		}
	}
};

using InstructionMap = std::map <Reference, InstructionSet>;

// TODO: persistent sbr structure that is updated instead of rebuilt
struct WholeBlockStructure {
	InstructionMap i2o;
	InstructionMap o2i;
	std::map <Reference, SharedBlockReference> blocks;
};

void build_usage_map(
	const SharedBlockReference &sbr,
	WholeBlockStructure &wbs
);

void add_to_usage_maps(
	const Reference &ref,
	WholeBlockStructure &wbs
)
{
	auto &i2o = wbs.i2o;
	auto &o2i = wbs.o2i;
	auto &operands = i2o.try_emplace(ref).first->second;

	auto add = [&](const Reference &opd) {
		if (not opd) return;
		operands.add(opd);
		auto &users = o2i.try_emplace(opd).first->second;
		users.add(ref);
	};

	ref->apply(add);

	// Nested blocks
	if (auto branch = ref->maybe <Branch> ()) {
		for (auto &b : branch->segments)
			build_usage_map(b.body, wbs);
		if (branch->fallback)
			build_usage_map(*branch->fallback, wbs);
	} else if (auto loop = ref->maybe <Loop> ()) {
		build_usage_map(loop->body, wbs);
	}
}

void build_usage_map(
	const SharedBlockReference &sbr,
	WholeBlockStructure &wbs
)
{
	auto &i2o = wbs.i2o;
	auto &o2i = wbs.o2i;
	for (auto &instr : *sbr) {
		wbs.blocks.try_emplace(instr, sbr);
		i2o.try_emplace(instr);
		o2i.try_emplace(instr);
		add_to_usage_maps(instr, wbs);
	}
}

auto build_whole_block_structure(const SharedBlockReference &sbr)
{
	WholeBlockStructure result;
	build_usage_map(sbr, result);
	return result;
}

void collect_blocks(
	const SharedBlockReference &sbr,
	std::vector <SharedBlockReference> &sbrs
)
{
	sbrs.emplace_back(sbr);
	for (auto &instr : *sbr) {
		if (auto branch = instr->maybe <Branch> ()) {
			for (auto &b : branch->segments)
				collect_blocks(b.body, sbrs);
			if (branch->fallback)
				collect_blocks(*branch->fallback, sbrs);
		} else if (auto loop = instr->maybe <Loop> ()) {
			collect_blocks(loop->body, sbrs);
		}
	}
}

// TODO: collect sbrs

bool dead_code_elimination_pass(const SharedBlockReference &sbr)
{
	auto wbs = build_whole_block_structure(sbr);

	std::set <Reference> remove;
	for (auto &[instr, uses] : wbs.o2i) {
		if (instr->is <Store> ()) {
			auto &store = instr->as <Store> ();
			auto &dst = store.destination;
			if (dst->is <Local> ()) {
				auto &uses = wbs.o2i.at(dst);
				if (uses.ordered.size() == 1)
					remove.insert(instr);
			}
		} else if (instr->is <Local> ()) {
			if (uses.ordered.empty())
				remove.insert(instr);
		}
	}
	
	std::vector <SharedBlockReference> blocks;
	collect_blocks(sbr, blocks);
	
	bool changed = false;
	for (auto &sbr : blocks) {
		changed |= std::erase_if(*sbr, [&](auto instr) {
			return remove.contains(instr);
		});
	}

	return changed;
}

bool local_store_elide(
	const WholeBlockStructure &wbs,
	const Reference &ref,
	const std::vector <Reference> &uses
)
{
	if (not ref->is <Local> ())
		return false;

	auto &local = ref->as <Local> ();
	if (local.init) {
		if (local.init == ref)
			return false;

		auto init_uses_it = wbs.o2i.find(local.init);
		if (init_uses_it == wbs.o2i.end())
			return false;

		auto &init_uses = init_uses_it->second;
		if (init_uses.ordered.size() != 1)
			return false;
		if (not init_uses.accounted.contains(ref))
			return false;

		std::vector <Reference> reads;
		for (auto &user : uses) {
			if (auto store = user->maybe <Store> ()) {
				if (store->destination == ref)
					return false;
			}

			reads.emplace_back(user);
		}

		size_t occurrences = 0;
		for (auto &user : reads) {
			user->apply([&](Reference &opd) {
				if (opd == ref)
					occurrences++;
			});
		}

		if (occurrences != 1)
			return false;

		auto value = local.init;
		bool changed = false;
		for (auto &user : reads) {
			user->apply([&](Reference &opd) {
				if (opd == ref) {
					opd = value;
					changed |= true;
				}
			});
		}
		return changed;
	}

	if (uses.size() < 2)
		return false;

	auto first = uses[0];
	if (not first->is <Store> ())
		return false;

	auto fstore = first->as <Store> ();
	if (fstore.destination != ref)
		return false;

	// Find the second store (if any)
	Reference second = nullptr;
	for (size_t i = 1; i < uses.size(); i++) {
		auto &user = uses[i];
		if (not user->is <Store> ())
			continue;

		auto &store = user->as <Store> ();
		if (store.destination == ref) {
			second = user;
			break;
		}
	}

	// For now only deal with useless locals
	if (second)
		return false;

	// TODO: may have to be careful if the source value changes across instructions?
	auto value = fstore.source;

	bool changed = false;
	for (size_t i = 1; i < uses.size(); i++) {
		auto &user = uses[i];
		user->apply([&](Reference &opd) {
			if (opd == ref) {
				opd = value;
				changed |= true;
			}
		});
	}

	return changed;
}

bool local_elision_pass(const SharedBlockReference &sbr)
{
	auto wbs = build_whole_block_structure(sbr);
	
	bool changed = false;
	for (auto &[instr, uses] : wbs.o2i)
		changed |= local_store_elide(wbs, instr, uses.ordered);
	return changed;
}

Primitive swizzle_type(Primitive base, SwizzleCode swizzle)
{
	const auto result_dim = std::string_view(repr(swizzle)).size();
	assertion(result_dim >= 1 && result_dim <= 4);

	const auto is_in_family = [&](Primitive vec2) {
		const auto offset = std::to_underlying(base) - std::to_underlying(vec2);
		return offset >= 0 && offset <= 2;
	};

	const auto map_family = [&](Primitive scalar, Primitive vec2) {
		if (result_dim == 1)
			return scalar;
		return Primitive(std::to_underlying(vec2) + (result_dim - 2));
	};

	if (is_in_family(Primitive::eUVec2))
		return map_family(Primitive::eUInt32, Primitive::eUVec2);
	if (is_in_family(Primitive::eIVec2))
		return map_family(Primitive::eInt32, Primitive::eIVec2);
	if (is_in_family(Primitive::eVec2))
		return map_family(Primitive::eFloat, Primitive::eVec2);

	fatal("unsupported swizzle type from base {} with {}", repr(base), repr(swizzle));
}

Reference get_or_add_type(const SharedBlockReference &sbr, const Type &request)
{
	for (auto &ref : *sbr) {
		if (ref->is <Type> () and ref->repr() == request.repr())
			return ref;
	}

	return sbr->add(0, request);
}

enum class PrimitiveFamily {
	eBool,
	eInt,
	eUInt,
	eFloat,
	eUnknown,
};

bool primitive_is_vector(Primitive prim)
{
	const auto value = std::to_underlying(prim);
	return (value >= std::to_underlying(Primitive::eUVec2)
			and value <= std::to_underlying(Primitive::eUVec4))
		or (value >= std::to_underlying(Primitive::eIVec2)
			and value <= std::to_underlying(Primitive::eIVec4))
		or (value >= std::to_underlying(Primitive::eVec2)
			and value <= std::to_underlying(Primitive::eVec4));
}

bool primitive_is_matrix(Primitive prim)
{
	const auto value = std::to_underlying(prim);
	return (value >= std::to_underlying(Primitive::eIMat2x2)
			and value <= std::to_underlying(Primitive::eIMat4x4))
		or (value >= std::to_underlying(Primitive::eUMat2x2)
			and value <= std::to_underlying(Primitive::eUMat4x4))
		or (value >= std::to_underlying(Primitive::eFMat2x2)
			and value <= std::to_underlying(Primitive::eFMat4x4));
}

bool primitive_is_scalar(Primitive prim)
{
	return prim == Primitive::eBool
		or prim == Primitive::eInt32
		or prim == Primitive::eUInt32
		or prim == Primitive::eFloat;
}

PrimitiveFamily primitive_family(Primitive prim)
{
	if (prim == Primitive::eBool)
		return PrimitiveFamily::eBool;

	if (prim == Primitive::eInt32
		or (prim >= Primitive::eIVec2 and prim <= Primitive::eIVec4)
		or (prim >= Primitive::eIMat2x2 and prim <= Primitive::eIMat4x4))
		return PrimitiveFamily::eInt;
	if (prim == Primitive::eUInt32
		or (prim >= Primitive::eUVec2 and prim <= Primitive::eUVec4)
		or (prim >= Primitive::eUMat2x2 and prim <= Primitive::eUMat4x4))
		return PrimitiveFamily::eUInt;
	if (prim == Primitive::eFloat
		or (prim >= Primitive::eVec2 and prim <= Primitive::eVec4)
		or (prim >= Primitive::eFMat2x2 and prim <= Primitive::eFMat4x4))
		return PrimitiveFamily::eFloat;

	return PrimitiveFamily::eUnknown;
}

size_t primitive_vector_dimension(Primitive prim)
{
	assertion(primitive_is_scalar(prim) or primitive_is_vector(prim));
	if (primitive_is_scalar(prim))
		return 1;

	if (prim >= Primitive::eUVec2 and prim <= Primitive::eUVec4)
		return std::to_underlying(prim) - std::to_underlying(Primitive::eUVec2) + 2;
	if (prim >= Primitive::eIVec2 and prim <= Primitive::eIVec4)
		return std::to_underlying(prim) - std::to_underlying(Primitive::eIVec2) + 2;
	if (prim >= Primitive::eVec2 and prim <= Primitive::eVec4)
		return std::to_underlying(prim) - std::to_underlying(Primitive::eVec2) + 2;

	fatal("failed to determine vector dimension for {}", repr(prim));
}

size_t primitive_matrix_columns(Primitive prim)
{
	assertion(primitive_is_matrix(prim));
	const auto base = [&]() {
		if (prim >= Primitive::eIMat2x2 and prim <= Primitive::eIMat4x4)
			return Primitive::eIMat2x2;
		if (prim >= Primitive::eUMat2x2 and prim <= Primitive::eUMat4x4)
			return Primitive::eUMat2x2;
		return Primitive::eFMat2x2;
	}();

	const auto offset = std::to_underlying(prim) - std::to_underlying(base);
	return static_cast <size_t> (2 + (offset / 3));
}

size_t primitive_matrix_rows(Primitive prim)
{
	assertion(primitive_is_matrix(prim));
	const auto base = [&]() {
		if (prim >= Primitive::eIMat2x2 and prim <= Primitive::eIMat4x4)
			return Primitive::eIMat2x2;
		if (prim >= Primitive::eUMat2x2 and prim <= Primitive::eUMat4x4)
			return Primitive::eUMat2x2;
		return Primitive::eFMat2x2;
	}();

	const auto offset = std::to_underlying(prim) - std::to_underlying(base);
	return static_cast <size_t> (2 + (offset % 3));
}

Primitive scalar_primitive(PrimitiveFamily family)
{
	switch (family) {
	case PrimitiveFamily::eBool:
		return Primitive::eBool;
	case PrimitiveFamily::eInt:
		return Primitive::eInt32;
	case PrimitiveFamily::eUInt:
		return Primitive::eUInt32;
	case PrimitiveFamily::eFloat:
		return Primitive::eFloat;
	default:
		break;
	}

	fatal("unsupported scalar primitive family");
}

Primitive vector_primitive(PrimitiveFamily family, size_t dim)
{
	assertion(dim >= 2 and dim <= 4);
	switch (family) {
	case PrimitiveFamily::eInt:
		return Primitive(std::to_underlying(Primitive::eIVec2) + (dim - 2));
	case PrimitiveFamily::eUInt:
		return Primitive(std::to_underlying(Primitive::eUVec2) + (dim - 2));
	case PrimitiveFamily::eFloat:
		return Primitive(std::to_underlying(Primitive::eVec2) + (dim - 2));
	default:
		break;
	}

	fatal("unsupported vector primitive family");
}

Primitive matrix_primitive(PrimitiveFamily family, size_t cols, size_t rows)
{
	assertion(cols >= 2 and cols <= 4);
	assertion(rows >= 2 and rows <= 4);

	const auto base = [&]() {
		switch (family) {
		case PrimitiveFamily::eInt:
			return Primitive::eIMat2x2;
		case PrimitiveFamily::eUInt:
			return Primitive::eUMat2x2;
		case PrimitiveFamily::eFloat:
			return Primitive::eFMat2x2;
		default:
			break;
		}
		fatal("unsupported matrix primitive family");
	}();

	return Primitive(std::to_underlying(base) + ((cols - 2) * 3 + (rows - 2)));
}

Reference get_or_add_type_ref(const SharedBlockReference &sbr, const Reference &ref);

Reference operation_type_ref(const SharedBlockReference &sbr, const Operation &opn)
{
	switch (opn.code) {
	case OperationCode::eEqual:
	case OperationCode::eGreater:
	case OperationCode::eGreaterEqual:
	case OperationCode::eLess:
	case OperationCode::eLessEqual:
	case OperationCode::eLogicalAnd:
	case OperationCode::eLogicalNot:
	case OperationCode::eLogicalOr:
	case OperationCode::eLogicalXor:
	case OperationCode::eNotEqual:
		return get_or_add_type(sbr, Primitive::eBool);
	default:
		break;
	}

	auto atype = get_or_add_type_ref(sbr, opn.a);
	if (not opn.b)
		return atype;

	if (opn.code != OperationCode::eMultiply)
		return atype;

	auto btype = get_or_add_type_ref(sbr, opn.b);
	auto &alhs = atype->as <Type> ();
	auto &arhs = btype->as <Type> ();

	if (not alhs.is <Primitive> () or not arhs.is <Primitive> ())
		return atype;

	auto lhs = alhs.as <Primitive> ();
	auto rhs = arhs.as <Primitive> ();
	if (primitive_family(lhs) != primitive_family(rhs))
		return atype;

	if (primitive_is_matrix(lhs) and primitive_is_vector(rhs)) {
		auto cols = primitive_matrix_columns(lhs);
		auto rows = primitive_matrix_rows(lhs);
		auto rdim = primitive_vector_dimension(rhs);
		if (cols == rdim)
			return get_or_add_type(sbr, vector_primitive(primitive_family(lhs), rows));
	}

	if (primitive_is_vector(lhs) and primitive_is_matrix(rhs)) {
		auto ldim = primitive_vector_dimension(lhs);
		auto rows = primitive_matrix_rows(rhs);
		auto cols = primitive_matrix_columns(rhs);
		if (ldim == rows)
			return get_or_add_type(sbr, vector_primitive(primitive_family(rhs), cols));
	}

	if (primitive_is_matrix(lhs) and primitive_is_matrix(rhs)) {
		auto lcols = primitive_matrix_columns(lhs);
		auto lrows = primitive_matrix_rows(lhs);
		auto rcols = primitive_matrix_columns(rhs);
		auto rrows = primitive_matrix_rows(rhs);
		if (lcols == rrows)
			return get_or_add_type(sbr, matrix_primitive(primitive_family(lhs), rcols, lrows));
	}

	return atype;
}

Reference builtin_intrinsic_type_ref(const SharedBlockReference &sbr, const BuiltinIntrinsic &bintr)
{
	auto arg_type = [&](size_t i) {
		assertion(i < bintr.args.size());
		return get_or_add_type_ref(sbr, bintr.args[i]);
	};

	switch (bintr.code) {
	case BuiltinIntrinsicCode::eBreak:
	case BuiltinIntrinsicCode::eContinue:
	case BuiltinIntrinsicCode::eDiscard:
	case BuiltinIntrinsicCode::eEmitMeshTasksEXT:
	case BuiltinIntrinsicCode::eSetMeshOutputsEXT:
		fatal("intrinsic {} has no value type", repr(bintr.code));
	case BuiltinIntrinsicCode::eDot:
	case BuiltinIntrinsicCode::eLength: {
		auto &type = arg_type(0)->as <Type> ();
		assertion(type.is <Primitive> ());
		auto family = primitive_family(type.as <Primitive> ());
		return get_or_add_type(sbr, scalar_primitive(family));
	}
	case BuiltinIntrinsicCode::eSample: {
		auto &type = arg_type(0)->as <Type> ();
		assertion(type.is <Primitive> ());
		auto family = primitive_family(type.as <Primitive> ());
		return get_or_add_type(sbr, vector_primitive(family, 4));
	}
	case BuiltinIntrinsicCode::eSelect:
		return arg_type(1);
	case BuiltinIntrinsicCode::eToFloat:
		return get_or_add_type(sbr, Primitive::eFloat);
	default:
		break;
	}

	return arg_type(0);
}

Reference get_or_add_type_ref(const SharedBlockReference &sbr, const Reference &ref)
{
	vswitch (*ref) {
	vcase(Argument): {
		auto &arg = ref->as <Argument> ();
		return get_or_add_type_ref(sbr, arg.type);
	}
	vcase(BuiltinIntrinsic): {
		auto &bintr = ref->as <BuiltinIntrinsic> ();
		return builtin_intrinsic_type_ref(sbr, bintr);
	}
	vcase(Constant): {
		auto &constant = ref->as <Constant> ();
		return std::visit([&] <typename T> (T x) -> Reference {
			using U = std::remove_cvref_t <T>;
			if constexpr (std::is_same_v <U, bool>)
				return get_or_add_type(sbr, Primitive::eBool);
			else if constexpr (std::is_same_v <U, int32_t>)
				return get_or_add_type(sbr, Primitive::eInt32);
			else if constexpr (std::is_same_v <U, uint32_t>)
				return get_or_add_type(sbr, Primitive::eUInt32);
			else if constexpr (std::is_same_v <U, float>)
				return get_or_add_type(sbr, Primitive::eFloat);
			else
				fatal("unsupported constant type in get_or_add_type_ref");
		}, constant);
	}
	vcase(Construct): {
		auto &ctor = ref->as <Construct> ();
		return get_or_add_type_ref(sbr, ctor.type);
	}
	vcase(Local): {
		auto &local = ref->as <Local> ();
		return get_or_add_type_ref(sbr, local.type);
	}
	vcase(Type): {
		return ref;
	}
	vcase(GlobalResource): {
		auto &grsrc = ref->as <GlobalResource> ();
		return get_or_add_type_ref(sbr, grsrc.type);
	}
	vcase(ArrayAccess): {
		auto &aacc = ref->as <ArrayAccess> ();
		auto &type = get_or_add_type_ref(sbr, aacc.value)->as <Type> ();
		assertion(type.is <Array> ());
		return get_or_add_type_ref(sbr, type.as <Array> ().base);
	}
	vcase(FieldAccess): {
		auto &facc = ref->as <FieldAccess> ();
		auto &type = get_or_add_type_ref(sbr, facc.value)->as <Type> ();
		assertion(type.is <Struct> ());
		return get_or_add_type_ref(sbr, type.as <Struct>().at(facc.fidx));
	}
	vcase(SystemValue): {
		auto &sv = ref->as <SystemValue> ();
		switch (sv) {
		case SystemValue::eClipPosition:
			return get_or_add_type(sbr, Primitive::eVec4);
		case SystemValue::eGlobalInvocationID:
		case SystemValue::eLocalInvocationID:
		case SystemValue::eWorkGroupID:
			return get_or_add_type(sbr, Primitive::eUVec3);
		case SystemValue::eInstanceIndex:
		case SystemValue::eVertexIndex:
			return get_or_add_type(sbr, Primitive::eUInt32);
		case SystemValue::eTaskPayload:
			return get_or_add_type_ref(sbr, sbr->task_payload_type.value());
		default:
			break;
		}
		break;
	}
	vcase(Operation): {
		auto &opn = ref->as <Operation> ();
		return operation_type_ref(sbr, opn);
	}
	vcase(Return): {
		auto &ret = ref->as <Return> ();
		return get_or_add_type_ref(sbr, ret.type);
	}
	vcase(StageInput): {
		auto &sin = ref->as <StageInput> ();
		return get_or_add_type_ref(sbr, sin.type);
	}
	vcase(StageOutput): {
		auto &sout = ref->as <StageOutput> ();
		return get_or_add_type_ref(sbr, sout.type);
	}
	vcase(Swizzle): {
		auto &swz = ref->as <Swizzle> ();
		auto type = get_or_add_type_ref(sbr, swz.value)->as <Type> ();
		assertion(type.is <Primitive> ());
		auto prim = type.as <Primitive> ();
		return get_or_add_type(sbr, swizzle_type(prim, swz.code));
	}
	default:
		break;
	}

	fatal("unhandled instruction in get_or_add_type_ref: {}", ref->repr());
}

bool has_only_local_init_uses(const Reference &ref, const std::vector <Reference> &uses)
{
	if (uses.empty())
		return false;

	for (auto &user : uses) {
		if (not user->is <Local> ())
			return false;

		auto &local = user->as <Local> ();
		if (local.init != ref)
			return false;
	}
	return true;
}

bool has_side_effects(const BuiltinIntrinsicCode code)
{
	switch (code) {
	case BuiltinIntrinsicCode::eBreak:
	case BuiltinIntrinsicCode::eContinue:
	case BuiltinIntrinsicCode::eDiscard:
	case BuiltinIntrinsicCode::eEmitMeshTasksEXT:
	case BuiltinIntrinsicCode::eSetMeshOutputsEXT:
		return true;
	default:
		break;
	}

	return false;
}

bool is_promotable_expression(const Reference &ref)
{
	vswitch (*ref) {
	vcase(Argument):
	vcase(ArrayAccess):
	vcase(Constant):
	vcase(Construct):
	vcase(FieldAccess):
	vcase(GlobalResource):
	vcase(Local):
	vcase(Operation):
	vcase(Return):
	vcase(StageInput):
	vcase(StageOutput):
	vcase(Swizzle):
	vcase(SystemValue): {
		return true;
	}
	vcase(BuiltinIntrinsic): {
		return not has_side_effects(ref->as <BuiltinIntrinsic> ().code);
	}
	default:
		break;
	}

	return false;
}

size_t expression_tree_size(
	const Reference &ref,
	std::map <Reference, size_t> &memo
)
{
	if (not ref)
		return 0;

	if (memo.contains(ref))
		return memo.at(ref);

	size_t size = 1;
	vswitch (*ref) {
	vcase(ArrayAccess): {
		auto &aacc = ref->as <ArrayAccess> ();
		size += expression_tree_size(aacc.value, memo);
		size += expression_tree_size(aacc.index, memo);
		break;
	}
	vcase(BuiltinIntrinsic): {
		auto &bintr = ref->as <BuiltinIntrinsic> ();
		for (auto &arg : bintr.args)
			size += expression_tree_size(arg, memo);
		break;
	}
	vcase(Construct): {
		auto &ctor = ref->as <Construct> ();
		for (auto &arg : ctor.args)
			size += expression_tree_size(arg, memo);
		break;
	}
	vcase(FieldAccess): {
		auto &facc = ref->as <FieldAccess> ();
		size += expression_tree_size(facc.value, memo);
		break;
	}
	vcase(Operation): {
		auto &opn = ref->as <Operation> ();
		size += expression_tree_size(opn.a, memo);
		size += expression_tree_size(opn.b, memo);
		break;
	}
	vcase(Swizzle): {
		auto &swz = ref->as <Swizzle> ();
		size += expression_tree_size(swz.value, memo);
		break;
	}
	default:
		break;
	}

	memo.emplace(ref, size);
	return size;
}

template <typename F>
void for_each_expression_operand(const Reference &ref, F &&ftn)
{
	auto &&fn = ftn;
	vswitch (*ref) {
	vcase(ArrayAccess): {
		auto &aacc = ref->as <ArrayAccess> ();
		fn(aacc.value);
		fn(aacc.index);
		break;
	}
	vcase(BuiltinIntrinsic): {
		auto &bintr = ref->as <BuiltinIntrinsic> ();
		for (auto &arg : bintr.args)
			fn(arg);
		break;
	}
	vcase(Branch): {
		auto &branch = ref->as <Branch> ();
		for (auto &segment : branch.segments)
			fn(segment.cond);
		break;
	}
	vcase(Construct): {
		auto &ctor = ref->as <Construct> ();
		for (auto &arg : ctor.args)
			fn(arg);
		break;
	}
	vcase(FieldAccess): {
		auto &facc = ref->as <FieldAccess> ();
		fn(facc.value);
		break;
	}
	vcase(Invocation): {
		auto &inv = ref->as <Invocation> ();
		for (auto &arg : inv.args)
			fn(arg);
		break;
	}
	vcase(Local): {
		auto &local = ref->as <Local> ();
		fn(local.init);
		break;
	}
	vcase(Operation): {
		auto &opn = ref->as <Operation> ();
		fn(opn.a);
		fn(opn.b);
		break;
	}
	vcase(Store): {
		auto &store = ref->as <Store> ();
		fn(store.source);
		break;
	}
	vcase(Swizzle): {
		auto &swz = ref->as <Swizzle> ();
		fn(swz.value);
		break;
	}
	default:
		break;
	}
}

bool is_readability_root(const Reference &ref)
{
	if (ref->is <Local> ())
		return true;
	if (ref->is <Store> ())
		return true;
	if (ref->is <Branch> ())
		return true;
	if (ref->is <Loop> ())
		return true;
	if (ref->is <Invocation> ())
		return true;
	if (ref->is <BuiltinIntrinsic> ()) {
		auto &bintr = ref->as <BuiltinIntrinsic> ();
		return has_side_effects(bintr.code);
	}

	return false;
}

void assign_readability_depth(
	const Reference &ref,
	size_t depth,
	std::map <Reference, size_t> &depths
)
{
	if (not ref)
		return;

	auto it = depths.find(ref);
	if (it != depths.end() and it->second >= depth)
		return;

	depths[ref] = depth;
	for_each_expression_operand(ref, [&](const Reference &opd) {
		assign_readability_depth(opd, depth + 1, depths);
	});
}

std::map <Reference, size_t> readability_depth_map(const SharedBlockReference &sbr)
{
	std::vector <SharedBlockReference> blocks;
	collect_blocks(sbr, blocks);

	std::map <Reference, size_t> depths;
	for (auto &block : blocks) {
		for (auto &instr : *block) {
			if (not is_readability_root(instr))
				continue;

			for_each_expression_operand(instr, [&](const Reference &opd) {
				assign_readability_depth(opd, 1, depths);
			});
		}
	}

	return depths;
}

std::map <Reference, size_t> instruction_order_map(const SharedBlockReference &sbr)
{
	std::vector <SharedBlockReference> blocks;
	collect_blocks(sbr, blocks);

	std::map <Reference, size_t> order;
	size_t index = 0;
	for (auto &block : blocks) {
		for (auto &instr : *block)
			order.emplace(instr, index++);
	}

	return order;
}

void promote_to_local(const WholeBlockStructure &wbs, const Reference &promote)
{
	auto &origin = wbs.blocks.at(promote);

	auto type = get_or_add_type_ref(origin, promote);
	auto index = std::find(origin->begin(), origin->end(), promote) - origin->begin();
	auto local = origin->add(index + 1, Local { type, promote });

	auto &uses = wbs.o2i.at(promote).ordered;
	for (auto &user : uses) {
		user->apply([&](Reference &opd) {
			if (opd == promote)
				opd = local;
		});
	}
}

using RefMetric = std::pair <Reference, size_t>;

bool reuse_pass(const SharedBlockReference &sbr)
{
	bool changed = false;

	while (true) {
		auto wbs = build_whole_block_structure(sbr);

		std::vector <RefMetric> sea;
		for (auto &[opd, uses] : wbs.o2i)
			sea.emplace_back(opd, uses.ordered.size());

		std::ranges::sort(sea, std::ranges::greater(), &RefMetric::second);

		Reference promote = nullptr;
		for (auto &[ref, count] : sea) {
			if (ref->is <Argument> ()
				or ref->is <Local> ()
				or ref->is <GlobalResource> ()
				or ref->is <Type> ())
				continue;

			if (count < 2)
				continue;

			promote = ref;
			break;

			// TODO: skip intrinsics with side effects
		}

		if (not promote)
			break;

		promote_to_local(wbs, promote);
		changed = true;
	}

	return changed;
}

void readability_pass(const SharedBlockReference &sbr)
{
	// Promote long expression trees into locals
	constexpr size_t long_expression_threshold = 5;
	while (true) {
		auto wbs = build_whole_block_structure(sbr);
		std::map <Reference, size_t> tree_sizes;
		auto depths = readability_depth_map(sbr);
		auto order = instruction_order_map(sbr);

		std::vector <Reference> sea;
		for (auto &[opd, uses] : wbs.o2i) {
			if (not is_promotable_expression(opd))
				continue;
			if (uses.ordered.empty())
				continue;
			if (has_only_local_init_uses(opd, uses.ordered))
				continue;

			auto size = expression_tree_size(opd, tree_sizes);
			if (size < long_expression_threshold)
				continue;

			sea.emplace_back(opd);
		}

		std::ranges::sort(sea, [&](const Reference &lhs, const Reference &rhs) {
			auto ldepth = depths.contains(lhs) ? depths.at(lhs) : 0;
			auto rdepth = depths.contains(rhs) ? depths.at(rhs) : 0;
			if (ldepth != rdepth)
				return ldepth > rdepth;

			auto lsize = tree_sizes.at(lhs);
			auto rsize = tree_sizes.at(rhs);
			if (lsize != rsize)
				return lsize > rsize;

			return order.at(lhs) < order.at(rhs);
		});
		if (sea.empty())
			break;

		promote_to_local(wbs, sea[0]);
	}
}

void optimize(const SharedBlockReference &sbr, OptimizationPhases phases)
{
	while (true) {
		bool changed = false;
		if (has_flag(phases, OptimizationPhases::eDeadCodeElimination))
			changed |= dead_code_elimination_pass(sbr);
		if (has_flag(phases, OptimizationPhases::eLocalElision))
			changed |= local_elision_pass(sbr);
		if (has_flag(phases, OptimizationPhases::eDeadCodeElimination))
			changed |= dead_code_elimination_pass(sbr);
		if (has_flag(phases, OptimizationPhases::eReuse))
			changed |= reuse_pass(sbr);

		if (not changed)
			break;
	}

	if (has_flag(phases, OptimizationPhases::eReadability))
		readability_pass(sbr);
}

} // namespace rcgp
