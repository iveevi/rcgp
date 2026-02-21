#include <map>
#include <print>
#include <set>
#include <algorithm>
#include <string_view>

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

bool local_store_elide(const Reference &ref, const std::vector <Reference> &uses)
{
	if (not ref->is <Local> ())
		return false;

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
		changed |= local_store_elide(instr, uses.ordered);
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

Reference get_or_add_type_ref(const SharedBlockReference &sbr, const Reference &ref)
{
	vswitch (*ref) {
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
		case SystemValue::eTaskPayload:
			return get_or_add_type_ref(sbr, sbr->task_payload_type.value());
		case SystemValue::eWorkGroupID:
			return get_or_add_type(sbr, Primitive::eUVec3);
		default:
			break;
		}
		break;
	}
	vcase(Operation): {
		// TODO: overload resolution
		auto &opn = ref->as <Operation> ();
		return get_or_add_type_ref(sbr, opn.a);
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

void readability_pass(const SharedBlockReference &sbr)
{
	while (true) {
		// Promote commonly used expressions into locals
		auto wbs = build_whole_block_structure(sbr);
		
		struct Counted {
			Reference ref;
			size_t count;
		};

		std::vector <Counted> sea;
		for (auto &[opd, uses] : wbs.o2i)
			sea.emplace_back(opd, uses.ordered.size());

		std::ranges::sort(sea, std::ranges::greater(), &Counted::count);

		Reference promote = nullptr;
		for (auto &[ref, count] : sea) {
			if (ref->is <Local> ()
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

		// Promote to local
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

	// TODO: turn local -> store into locals with initializations

	// TODO: promote via heuristics about the length of the instruction
}

void optimize(const SharedBlockReference &sbr, OptimizationPhases phases)
{
	while (true) {
		bool changed = false;
		if (has_flag(phases, OptimizationPhases::eDeadCodeElimination))
			changed |= dead_code_elimination_pass(sbr);
		if (has_flag(phases, OptimizationPhases::eLocalElision))
			changed |= local_elision_pass(sbr);

		if (not changed)
			break;
	}

	if (has_flag(phases, OptimizationPhases::eReadability))
		readability_pass(sbr);
}

} // namespace rcgp
