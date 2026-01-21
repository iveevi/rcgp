#pragma once

#include <map>

#include "../pipeline/compute.hpp"
#include "../pipeline/mesh_shading.hpp"
#include "../pipeline/rasterization.hpp"

struct PipelineMappings {
	vk::PipelineLayout layout;
	vk::PipelineBindPoint bind_point;
	std::map <void *, size_t> pc_offsets;
	std::map <void *, vk::ShaderStageFlags> pc_stages;
	std::map <void *, size_t> vb_offsets;

	static inline std::map <size_t, PipelineMappings> cache;
};

template <auto &... refs>
void write_vb_offsets(PipelineMappings &dst, Tlist <reference <refs>...>)
{
	size_t counter = 0;
	(dst.vb_offsets.emplace(&refs, counter++), ...);
}

template <typename ... Wrappers>
void write_pb_infos(PipelineMappings &dst, Tlist <Wrappers...>)
{
	using GRCs = Tlist <Wrappers...>;
	auto write = [&] <auto &ref> () {
		using Resource = reference_base_t <ref>;

		if constexpr (is_push_constant_v <Resource>) {
			auto flags = stage_flags_for_v <ref, GRCs>;
			constexpr auto offset = push_constant_offset_for <ref> (GRCs());
			dst.pc_stages.emplace(&ref, flags);
			dst.pc_offsets.emplace(&ref, offset);
		}
	};

	(write.template operator() <Wrappers::reference::handle> (), ...);
}

template <Topology T, typename AS, typename GAMAP, typename GRCs>
auto pipeline_mappings(const RasterizationPipeline <T, AS, GAMAP, GRCs> &pipeline)
{
	PipelineMappings result;
	result.layout = pipeline.layout;
	result.bind_point = vk::PipelineBindPoint::eGraphics;
	write_vb_offsets(result, AS());
	write_pb_infos(result, GRCs());
	return result;
}

template <typename GAMAP, typename GRCs>
auto pipeline_mappings(const ComputePipeline <GAMAP, GRCs> &pipeline)
{
	PipelineMappings result;
	result.layout = pipeline.layout;
	result.bind_point = vk::PipelineBindPoint::eCompute;
	write_pb_infos(result, GRCs());
	return result;
}

template <typename GAMAP, typename GRCs>
auto pipeline_mappings(const MeshShadingPipeline <GAMAP, GRCs> &pipeline)
{
	PipelineMappings result;
	result.layout = pipeline.layout;
	result.bind_point = vk::PipelineBindPoint::eGraphics;
	write_pb_infos(result, GRCs());
	return result;
}
