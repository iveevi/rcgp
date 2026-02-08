#pragma once

#include "../rhi/descriptor_pool.hpp"
#include "descriptor.hpp"
#include "group_allocation.hpp"
#include "index_buffer.hpp"
#include "contract.hpp"

namespace rcgp {

template <auto &ref, auto &... refs, size_t ... Is>
constexpr auto set_index_for(const Tlist <group_allocation_record <refs, Is>...> &)
{
	constexpr auto matches = std::array {
		std::same_as <
			contract <ref>,
			contract <refs>
		>...
	};

	constexpr auto index = first_on(matches);
	if constexpr (index < 0) {
		static_error("contract not in group allocation"_ss);
		return 0;
	} else {
		return Is...[index];
	}
}

template <typename GAMAP, typename GRCs>
struct GenericPipeline {
	static constexpr size_t set_count = GAMAP::size;
	
	VkDevice device = VK_NULL_HANDLE;
	VkPipeline handle = VK_NULL_HANDLE;
	VkPipelineLayout layout = VK_NULL_HANDLE;
	std::array <VkDescriptorSetLayout, set_count> dsls;

	GenericPipeline() = default;

	GenericPipeline(
		VkDevice device_,
		VkPipeline handle_,
		VkPipelineLayout layout_,
		const std::array <VkDescriptorSetLayout, set_count> &dsls_
	) : device(device_), handle(handle_), layout(layout_), dsls(dsls_) {}

	// TODO: support allocating multiple refs
	template <auto &ref>
	auto new_descriptor(const DescriptorPool &pool) const {
		constexpr auto set = set_index_for <ref> (GAMAP());

		auto dsets = Device {
			.logical = device,
		}.new_descriptor_sets(pool, std::span <const VkDescriptorSetLayout> (&dsls[set], 1));

		return DescriptorFor <ref, false> (dsets.front(), set);
	}
};

template <Topology T, typename AS, typename GAMAP, typename GRCs>
struct RasterizationPipeline : GenericPipeline <GAMAP, GRCs> {
	using GenericPipeline <GAMAP, GRCs> ::GenericPipeline;
};

#define $rasterization_pipeline_t(T, vs, fs) \
	decltype(std::declval <RasterizationCombinator <Topology::T>> ()(vs, fs));

template <typename GAMAP, typename GRCs>
struct ComputePipeline : GenericPipeline <GAMAP, GRCs> {
	using GenericPipeline <GAMAP, GRCs> ::GenericPipeline;
};

template <typename GAMAP, typename GRCs>
struct MeshShadingPipeline : GenericPipeline <GAMAP, GRCs> {
	using GenericPipeline <GAMAP, GRCs> ::GenericPipeline;
};

} // namespace rcgp
