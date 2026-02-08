#pragma once

#include <cstdlib>

#include "process_wrappers.hpp"
#include "group_allocation.hpp"

namespace rcgp {

template <typename ... Ts>
auto wrappers_to_gamap(const Tlist <Ts...> &)
{
	auto alloc = constexpr_for(Is, sizeof...(Ts),
		return Tlist <group_allocation_record <Ts::contract::handle, Is>...> {}
	);
	
	auto gamap = [&] <typename ... Records> (Tlist <Records...>) {
		return group_allocation_map {
			{ Records::vptr, Records::index }...
		};
	} (alloc);

	return std::tuple { alloc, gamap };
}

template <typename GVRs, typename ... Blocks>
auto apply_gvrs(const Device &device, const GVRs &, Blocks &... blocks)
{
	using descriptor_gvrs = descriptable_resources_t <GVRs>;
	using push_constant_gvrs = push_constant_resources_t <GVRs>;

	auto [alloc, gamap] = wrappers_to_gamap(descriptor_gvrs());
	(blocks->apply_group_allocation_map(gamap), ...);

	auto [pcrs, pcmap] = wrappers_to_pcs(push_constant_gvrs());
	(blocks->apply_push_constant_allocation_map(pcmap), ...);

	auto dsls = wrappers_to_dsls(device, descriptor_gvrs());

	VkPipelineLayoutCreateInfo layout_info {};
	layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_info.setLayoutCount = dsls.size();
	layout_info.pSetLayouts = dsls.data();

	if constexpr (push_constant_gvrs::size > 0) {
		layout_info.pushConstantRangeCount = pcrs.size();
		layout_info.pPushConstantRanges = pcrs.data();
	}

	auto layout = VkPipelineLayout(VK_NULL_HANDLE);
	auto result = vkCreatePipelineLayout(device.logical, &layout_info, nullptr, &layout);
	if (result != VK_SUCCESS)
		std::abort();

	return std::tuple {
		layout,
		dsls,
		alloc,
	};
}

} // namespace rcgp
