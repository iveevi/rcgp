#pragma once

#include "../dsl/block.hpp"
#include "index_buffer.hpp"

namespace rcgp {

template <typename GAMAP, typename GRCs>
struct GenericPipeline {
	static constexpr size_t set_count = GAMAP::size;
	
	vk::Pipeline handle;
	vk::PipelineLayout layout;
	std::array <vk::DescriptorSetLayout, set_count> dsls;
	group_allocation_map gamap;

	GenericPipeline() = default;
	GenericPipeline(
		const vk::Pipeline &handle_,
		const vk::PipelineLayout &layout_,
		const std::array <vk::DescriptorSetLayout, set_count> &dsls_,
		const group_allocation_map &gamap_
	) : handle(handle_), layout(layout_), dsls(dsls_), gamap(gamap_) {}
};

template <Topology T, typename AS, typename GAMAP, typename GRCs>
struct RasterizationPipeline : GenericPipeline <GAMAP, GRCs> {
	using GenericPipeline <GAMAP, GRCs> ::GenericPipeline;
};

template <typename GAMAP, typename GRCs>
struct ComputePipeline : GenericPipeline <GAMAP, GRCs> {
	using GenericPipeline <GAMAP, GRCs> ::GenericPipeline;
};

template <typename GAMAP, typename GRCs>
struct MeshShadingPipeline : GenericPipeline <GAMAP, GRCs> {
	using GenericPipeline <GAMAP, GRCs> ::GenericPipeline;
};

// Type trait for pipelines
TYPE_TRAIT(is_pipeline);
	template <Topology T, typename AS, typename GAMAP, typename GRCs>
	TYPE_TRAIT_INCLUDES(is_pipeline, RasterizationPipeline <T, AS, GAMAP, GRCs>);

	template <typename GA, typename GR>
	TYPE_TRAIT_INCLUDES(is_pipeline, ComputePipeline <GA, GR>);

	template <typename GA, typename GR>
	TYPE_TRAIT_INCLUDES(is_pipeline, MeshShadingPipeline <GA, GR>);

} // namespace rcgp
