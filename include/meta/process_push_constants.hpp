#pragma once

#include <array>

#include "../dsl/block.hpp"
#include "../util/align.hpp"
#include "../util/cti.hpp"
#include "witnesses.hpp"
#include "resources.hpp"
#include "resources_collect.hpp"
#include "shader_stage_conversion.hpp"

namespace rcgp {

template <auto &ref, typename ... Wrappers>
consteval size_t push_constant_offset_for(Tlist <Wrappers...>)
{
	size_t offset = 0;
	bool passed = false;
	auto accum = [&] <typename W> () {
		using R = W::type;
		using T = ResourceTypeFor <W::handle>;

		if constexpr (std::same_as <typename W::contract, contract <ref>>)
			passed = true;
		if (not passed && is_push_constant_v <R>) {
			offset = align_up(offset, alignof(T));
			offset += sizeof(T);
		}
	};
	(accum.template operator() <Wrappers> (), ...);
	return offset;
}

template <typename ... Wrappers>
auto wrappers_to_pcs(const Tlist <Wrappers...> &)
{
	push_constant_allocation_map map;
	if constexpr (sizeof...(Wrappers) == 0) {
		std::array <vk::PushConstantRange, 0> ranges;
		return std::tuple { ranges, map };
	} else {
		std::array <vk::PushConstantRange, sizeof...(Wrappers)> ranges {};

		uint32_t offset = 0;
		uint32_t index = 0;

		auto populate = [&] <typename W> () {
			using T = ResourceTypeFor <W::handle>;

			offset = align_up(static_cast <size_t> (offset), alignof(T));
			
			ranges[index++] = vk::PushConstantRange()
				.setOffset(offset)
				.setSize(sizeof(T))
				.setStageFlags(W::flags);

			map.emplace(W::contract::address, offset);

			offset += sizeof(T);
		};

		(populate.template operator() <Wrappers> (), ...);

		return std::tuple { ranges, map };
	}
}

} // namespace rcgp
