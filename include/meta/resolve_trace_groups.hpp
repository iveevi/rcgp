#pragma once

#include <map>
#include <vector>

#include "../dsl/block.hpp"
#include "../dsl/instructions.hpp"
#include "../util/error.hpp"

namespace rcgp {

template <typename ... MissShaders>
void resolve_trace_groups(
	const SharedBlockReference &rgen,
	const std::tuple <MissShaders...> &misses,
	auto &... chit_blocks
)
{
	// 1. Build miss map: trace_group_addr -> miss_shader_index
	//    by inspecting each miss shader's global_resources for eRayReceiverPayload
	std::map <void *, uint32_t> miss_map;

	auto register_miss = [&] <size_t... Is> (std::index_sequence <Is...>) {
		auto process = [&](auto &miss_shader, uint32_t index) {
			for (auto &[addr, refs] : miss_shader->global_resources) {
				for (auto &ref : refs) {
					if (not ref->template is <GlobalResource> ())
						continue;
					auto &grsrc = ref->template as <GlobalResource> ();
					if (grsrc.kind == GlobalResourceKind::eRayReceiverPayload)
						miss_map.emplace(addr, index);
				}
			}
		};

		(process(std::get <Is> (misses), uint32_t(Is)), ...);
	};

	register_miss(std::index_sequence_for <MissShaders...> {});

	// 2. Build payload index map: trace_group_addr -> payload_index
	//    Collect from all blocks' trace_groups maps and global_resources
	std::map <void *, uint32_t> payload_map;
	uint32_t next_payload = 0;

	auto assign_payload = [&](void *addr) {
		if (not payload_map.contains(addr))
			payload_map.emplace(addr, next_payload++);
	};

	// From raygen trace_groups (dispatchers)
	for (auto &[addr, _] : rgen->trace_groups)
		assign_payload(addr);

	// From miss shaders (receivers)
	for (auto &[addr, _] : miss_map)
		assign_payload(addr);

	// From chit blocks (receivers)
	auto assign_from_chit = [&](auto &block) {
		for (auto &[addr, refs] : block->global_resources) {
			for (auto &ref : refs) {
				if (not ref->template is <GlobalResource> ())
					continue;
				auto &grsrc = ref->template as <GlobalResource> ();
				if (grsrc.kind == GlobalResourceKind::eRayReceiverPayload)
					assign_payload(addr);
			}
		}
	};

	(assign_from_chit(chit_blocks), ...);

	// 3. Resolve eTraceRaysEXT intrinsics in all blocks that have trace_groups
	auto resolve_block = [&](this auto &self, const SharedBlockReference &block) -> void {
		for (auto &[addr, trace_calls] : block->trace_groups) {
			auto miss_it = miss_map.find(addr);
			assertion(miss_it != miss_map.end());

			auto payload_it = payload_map.find(addr);
			assertion(payload_it != payload_map.end());

			for (auto &ref : trace_calls) {
				auto &bintr = ref->as <BuiltinIntrinsic> ();
				assertion(bintr.code == BuiltinIntrinsicCode::eTraceRaysEXT);
				assertion(bintr.args.size() == 11);

				// args[5] = miss index
				bintr.args[5] = std::make_shared <Instruction> (
					Constant(int32_t(miss_it->second))
				);

				// args[10] = payload index
				bintr.args[10] = std::make_shared <Instruction> (
					Constant(int32_t(payload_it->second))
				);
			}
		}

		// Recurse into nested blocks (branches, loops)
		for (auto &ref : *block) {
			if (ref->is <Branch> ()) {
				auto &branch = ref->as <Branch> ();
				for (auto &seg : branch.segments)
					self(seg.body);
				if (branch.fallback)
					self(*branch.fallback);
			} else if (ref->is <Loop> ()) {
				self(ref->as <Loop> ().body);
			}
		}
	};

	resolve_block(rgen);
	(resolve_block(chit_blocks), ...);

	// 4. Apply payload allocation map to all shader blocks
	rgen->apply_ray_payload_allocation_map(payload_map);
	[&] <size_t... Is> (std::index_sequence <Is...>) {
		(std::get <Is> (misses)->apply_ray_payload_allocation_map(payload_map), ...);
	}(std::index_sequence_for <MissShaders...> {});
	(chit_blocks->apply_ray_payload_allocation_map(payload_map), ...);
}

} // namespace rcgp
