#include <algorithm>
#include <map>
#include <set>
#include <vector>

#include "dsl/optimizer.hpp"
#include "dsl/instructions.hpp"
#include "util/timer.hpp"

struct LocalInfo {
	uint32_t read_count = 0;
	uint32_t write_count = 0;
	Reference store_src;
	const Instruction *store_instr = nullptr;
	const Block *store_block = nullptr;
	const Block *read_block = nullptr;
};

void collect_blocks(std::vector <SharedBlockReference> &blocks, const SharedBlockReference &root)
{
	std::set <const Block *> visited;

	auto visit = [&](auto &&self, const SharedBlockReference &blk) -> void {
		if (!blk)
			return;
		if (visited.contains(blk.get()))
			return;
		visited.emplace(blk.get());
		blocks.push_back(blk);

		for (auto &instr : *blk) {
			// TODO: use switch
			if (instr->is <Invocation> ()) {
				self(self, instr->as <Invocation> ().sbr);
			} else if (instr->is <Branch> ()) {
				auto &branch = instr->as <Branch> ();
				for (auto &segment : branch.segments)
					self(self, segment.body);
				if (branch.fallback.has_value())
					self(self, branch.fallback.value());
			} else if (instr->is <Loop> ()) {
				auto &loop = instr->as <Loop> ();
				if (loop.init.has_value())
					self(self, loop.init.value());
				self(self, loop.cond);
				if (loop.step.has_value())
					self(self, loop.step.value());
				self(self, loop.body);
			}
		}
	};

	visit(visit, root);
}

bool is_pure_expr(const Reference &ref)
{
	if (!ref)
		return true;
	if (ref->is <Store> () || ref->is <Invocation> ()
		|| ref->is <Branch> () || ref->is <Loop> ())
		return false;
	return true;
}

void visit_refs_mut(Reference &instr, const auto &fn)
{
	if (!instr)
		return;

	std::visit([&](auto &node) {
		using T = std::decay_t <decltype(node)>;
		if constexpr (std::is_same_v <T, Operation>) {
			fn(node.a);
			fn(node.b);
		} else if constexpr (std::is_same_v <T, Store>) {
			fn(node.destination);
			fn(node.source);
		} else if constexpr (std::is_same_v <T, ArrayAccess>) {
			fn(node.value);
			fn(node.index);
		} else if constexpr (std::is_same_v <T, FieldAccess>) {
			fn(node.value);
		} else if constexpr (std::is_same_v <T, Swizzle>) {
			fn(node.value);
		} else if constexpr (std::is_same_v <T, Invocation>) {
			for (auto &arg : node.args)
				fn(arg);
		} else if constexpr (std::is_same_v <T, Construct>) {
			fn(node.type);
			for (auto &arg : node.args)
				fn(arg);
		} else if constexpr (std::is_same_v <T, BuiltinIntrinsic>) {
			for (auto &arg : node.args)
				fn(arg);
		} else if constexpr (std::is_same_v <T, Argument>) {
			fn(node.type);
		} else if constexpr (std::is_same_v <T, ThreadInput>) {
			fn(node.type);
		} else if constexpr (std::is_same_v <T, ThreadOutput>) {
			fn(node.type);
		} else if constexpr (std::is_same_v <T, GlobalResource>) {
			fn(node.type);
		} else if constexpr (std::is_same_v <T, Branch>) {
			for (auto &segment : node.segments)
				fn(segment.cond);
		} else if constexpr (std::is_same_v <T, Loop>) {
			fn(node.cond_value);
		} else {
			// Constant, Local, GlobalIntrinsic, Block, etc.
		}
	}, *instr);
}

void optimize_block(const SharedBlockReference &sbr)
{
	TSCOPE("optimize block");

	if (!sbr)
		return;

	// TODO: this pattern shows up multiple times...
	std::vector <SharedBlockReference> blocks;
	collect_blocks(blocks, sbr);

	std::map <const Instruction *, const Block *> instr_blocks;
	std::map <const Instruction *, LocalInfo> locals;

	for (auto &blk : blocks) {
		for (auto &instr : *blk)
			instr_blocks.emplace(instr.get(), blk.get());
	}

	for (auto &blk : blocks) {
		for (auto &instr : *blk) {
			if (instr->is <Local> ())
				locals.emplace(instr.get(), LocalInfo {});
		}
	}

	for (auto &blk : blocks) {
		for (auto &instr : *blk) {
			if (instr->is <Store> ()) {
				auto &store = instr->as <Store> ();
				if (store.destination && store.destination->is <Local> ()) {
					auto &info = locals[store.destination.get()];
					info.write_count++;
					info.store_src = store.source;
					info.store_instr = instr.get();
					info.store_block = blk.get();
				}
				if (store.source && store.source->is <Local> ()) {
					auto &info = locals[store.source.get()];
					info.read_count++;
					info.read_block = blk.get();
				}
				continue;
			}

			visit_refs_mut(instr, [&](Reference &ref) {
				if (ref && ref->is <Local> ()) {
					auto &info = locals[ref.get()];
					info.read_count++;
					info.read_block = blk.get();
				}
			});
		}
	}

	std::map <Reference, Reference> replace;
	std::set <const Instruction *> remove;

	for (auto &[local_instr, info] : locals) {
		if (info.write_count == 0 && info.read_count == 0) {
			remove.insert(local_instr);
			continue;
		}

		if (info.write_count == 1 && info.read_count == 1
			&& info.store_instr && info.store_src
			&& is_pure_expr(info.store_src)
			&& info.store_block == info.read_block) {
			Reference local_ref;
			for (auto &blk : blocks) {
				for (auto &instr : *blk) {
					if (instr.get() == local_instr) {
						local_ref = instr;
						break;
					}
				}
				if (local_ref)
					break;
			}
			if (local_ref) {
				replace.emplace(local_ref, info.store_src);
				remove.insert(local_instr);
				remove.insert(info.store_instr);
			}
		}
	}

	if (replace.empty() && remove.empty())
		return;

	auto resolve = [&](Reference &ref) {
		auto it = replace.find(ref);
		while (it != replace.end()) {
			ref = it->second;
			it = replace.find(ref);
		}
	};

	for (auto &blk : blocks) {
		for (auto &instr : *blk) {
			visit_refs_mut(instr, [&](Reference &ref) {
				if (ref)
					resolve(ref);
			});
		}
	}

	for (auto &blk : blocks) {
		blk->erase(std::remove_if(
			blk->begin(),
			blk->end(),
			[&](const Reference &ref) {
				return remove.contains(ref.get());
			}),
			blk->end()
		);
	}
}
