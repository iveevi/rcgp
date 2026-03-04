#pragma once

#include <map>

#include "pipelines.hpp"
#include "process_push_constants.hpp"

namespace rcgp {

struct PipelineMappings {
	vk::PipelineLayout layout;
	vk::PipelineBindPoint bind_point;
	group_allocation_map gamap;
	std::map <void *, size_t> pc_offsets;
	std::map <void *, vk::ShaderStageFlags> pc_stages;
	std::map <void *, size_t> vb_offsets;

	static inline std::map <size_t, PipelineMappings> cache;
};

template <auto &... refs>
void write_vb_offsets(PipelineMappings &dst, Tlist <contract <refs>...>)
{
	size_t counter = 0;
	(dst.vb_offsets.emplace(&refs, counter++), ...);
}

// TODO: function...
template <auto &ref, typename Seq>
struct stage_flags_for_seq;

template <auto &ref>
struct stage_flags_for_seq <ref, Tlist <>> {
	static constexpr vk::ShaderStageFlags value = {};
};

template <auto &ref, auto &other, ShaderStage ...Ss, typename ...Rest>
struct stage_flags_for_seq <
	ref,
	Tlist <stage_wrapper <other, Ss...>, Rest...>
> {
	static constexpr vk::ShaderStageFlags value =
		std::same_as <contract <other>, contract <ref>>
			? stage_flags_of <Ss...> ()
			: stage_flags_for_seq <ref, Tlist <Rest...>> ::value;
};

template <auto &ref, typename Seq>
constexpr vk::ShaderStageFlags stage_flags_for_v = stage_flags_for_seq <ref, Seq> ::value;

template <typename ... Wrappers>
void write_pb_infos(PipelineMappings &dst, Tlist <Wrappers...>)
{
	using GRCs = Tlist <Wrappers...>;
	auto write = [&] <auto &ref> () {
		using Resource = reference_base_of <ref>;

		if constexpr (is_push_constant_v <Resource>) {
			auto flags = stage_flags_for_v <ref, GRCs>;
			constexpr auto offset = push_constant_offset_for <ref> (GRCs());
			dst.pc_stages.emplace(&ref, flags);
			dst.pc_offsets.emplace(&ref, offset);
		}
	};

	(write.template operator() <Wrappers::contract::handle> (), ...);
}

template <Topology T, typename AS, typename GAMAP, typename GRCs>
auto pipeline_mappings(const RasterizationPipeline <T, AS, GAMAP, GRCs> &pipeline)
{
	PipelineMappings result;
	result.layout = pipeline.layout;
	result.bind_point = vk::PipelineBindPoint::eGraphics;
	write_vb_offsets(result, AS());
	write_pb_infos(result, GRCs());
	result.gamap = pipeline.gamap;
	return result;
}

template <typename GAMAP, typename GRCs>
auto pipeline_mappings(const ComputePipeline <GAMAP, GRCs> &pipeline)
{
	PipelineMappings result;
	result.layout = pipeline.layout;
	result.bind_point = vk::PipelineBindPoint::eCompute;
	write_pb_infos(result, GRCs());
	result.gamap = pipeline.gamap;
	return result;
}

template <typename GAMAP, typename GRCs>
auto pipeline_mappings(const MeshShadingPipeline <GAMAP, GRCs> &pipeline)
{
	PipelineMappings result;
	result.layout = pipeline.layout;
	result.bind_point = vk::PipelineBindPoint::eGraphics;
	write_pb_infos(result, GRCs());
	result.gamap = pipeline.gamap;
	return result;
}

} // namespace rcgp
