#include <map>
#include <print>
#include <set>

#include "dsl/block.hpp"
#include "dsl/instructions.hpp"
#include "dsl/optimization.hpp"

namespace rcgp {

struct InstructionSet {
	std::set <Instruction *> accounted;
	std::vector <Instruction *> ordered;

	void add(Instruction *instr) {
		if (not accounted.contains(instr)) {
			accounted.insert(instr);
			ordered.emplace_back(instr);
		}
	}
};

using InstructionMap = std::map <const Instruction *, InstructionSet>;

void build_usage_map(
	const SharedBlockReference &sbr,
	InstructionMap &instr_to_opds,
	InstructionMap &opd_to_instr
);

void add_to_usage_maps(
	const Reference &ref,
	InstructionMap &i2o,
	InstructionMap &o2i
)
{
	auto pinstr = ref.get();
	auto add = [&](const Reference &opd) {
		if (not opd) return;
		auto popd = opd.get();
		i2o.at(pinstr).add(popd);
		o2i.at(popd).add(pinstr);
	};

	ref->apply(add);

	// Nested blocks
	if (auto branch = ref->maybe <Branch> ()) {
		for (auto &b : branch->segments)
			build_usage_map(b.body, i2o, o2i);
		if (branch->fallback)
			build_usage_map(*branch->fallback, i2o, o2i);
	} else if (auto loop = ref->maybe <Loop> ()) {
		build_usage_map(loop->body, i2o, o2i);
	}
}

void build_usage_map(
	const SharedBlockReference &sbr,
	InstructionMap &i2o,
	InstructionMap &o2i
)
{
	for (auto &instr : *sbr) {
		auto pinstr = instr.get();
		if (not i2o.contains(pinstr))
			i2o.emplace(pinstr, InstructionSet());
		if (not o2i.contains(pinstr))
			o2i.emplace(pinstr, InstructionSet());

		add_to_usage_maps(instr, i2o, o2i);
	}
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
	InstructionMap i2o;
	InstructionMap o2i;
	build_usage_map(sbr, i2o, o2i);
	
	std::vector <SharedBlockReference> blocks;
	collect_blocks(sbr, blocks);

	bool changed = false;
	for (auto &sbr : blocks) {
		std::set <Reference> remove;
		for (auto &instr : *sbr) {
			if (instr->is <Store> ()) {
				auto &store = instr->as <Store> ();
				auto &dst = store.destination;
				if (dst->is <Local> ()) {
					auto &uses = o2i.at(dst.get());
					if (uses.ordered.size() == 1)
						remove.insert(instr);
				}
			} else if (instr->is <Local> ()) {
				auto &uses = o2i.at(instr.get());
				if (uses.ordered.empty())
					remove.insert(instr);
			}
		}

		changed |= std::erase_if(*sbr, [&](auto instr) {
			return remove.contains(instr);
		});
	}

	return changed;
}

bool local_store_elide(const Reference &instr, const InstructionMap &o2i)
{
	if (not instr->is <Local> ())
		return false;

	auto uses = o2i.at(instr.get()).ordered;
	if (uses.size() < 2)
		return false;

	auto first = uses[0];
	if (not first->is <Store> ())
		return false;

	auto fstore = first->as <Store> ();
	if (fstore.destination != instr)
		return false;

	// Find the second store (if any)
	const Instruction *second = nullptr;
	for (size_t i = 1; i < uses.size(); i++) {
		auto &user = uses[i];
		if (not user->is <Store> ())
			continue;

		auto &store = user->as <Store> ();
		if (store.destination == instr) {
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
			if (opd == instr) {
				opd = value;
				changed |= true;
			}
		});
	}

	return changed;
}

bool local_elision_pass(const SharedBlockReference &sbr)
{
	bool changed = false;
	
	InstructionMap i2o;
	InstructionMap o2i;
	build_usage_map(sbr, i2o, o2i);

	std::vector <SharedBlockReference> blocks;
	collect_blocks(sbr, blocks);

	for (auto &sbr : blocks) {
		for (auto &instr : *sbr)
			changed |= local_store_elide(instr, o2i);
	}

	return changed;
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
}

} // namespace rcgp
