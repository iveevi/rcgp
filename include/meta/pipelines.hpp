#pragma once

#include "../rhi/descriptor_pool.hpp"
#include "descriptor.hpp"
#include "group_allocation.hpp"
#include "index_buffer.hpp"
#include "reference.hpp"

namespace rcgp {

template <auto &ref, auto &... refs, size_t ... Is>
constexpr auto set_index_for(const Tlist <group_allocation_record <refs, Is>...> &)
{
	// TODO: use constexpr_for?
	constexpr auto matches = std::array {
		std::same_as <
			reference <ref>,
			reference <refs>
		>...
	};

	constexpr auto index = first_on(matches);
	if constexpr (index < 0) {
		static_error("reference not in group allocation"_ss);
		return 0;
	} else {
		return Is...[index];
	}
}

template <typename GAMAP, typename GRCs>
struct GenericPipeline {
	static constexpr size_t set_count = GAMAP::size;
	
	vk::Device device;
	vk::Pipeline handle;
	vk::PipelineLayout layout;
	std::array <vk::DescriptorSetLayout, set_count> dsls;

	GenericPipeline() = default;

	GenericPipeline(
		const vk::Device &device_,
		const vk::Pipeline &handle_,
		const vk::PipelineLayout &layout_,
		const std::array <vk::DescriptorSetLayout, set_count> &dsls_
	) : device(device_), handle(handle_), layout(layout_), dsls(dsls_) {}

	template <auto &ref>
	auto new_descriptor(const DescriptorPool &pool) const {
		constexpr auto set = set_index_for <ref> (GAMAP());
		
		auto dset = device.allocateDescriptorSets(
			vk::DescriptorSetAllocateInfo()
				.setDescriptorPool(pool)
				.setSetLayouts(dsls[set])
		).front();

		return DescriptorFor <ref, false> (dset, set);
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
