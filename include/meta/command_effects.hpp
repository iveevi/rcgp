#pragma once

#include "contract.hpp"
#include "pipelines.hpp"

namespace rcgp {

// We have a dependency on ref
template <auto &ref>
struct Dependency {
	static constexpr auto &handle = ref;
};

// We *might have a dependency for an index buffer for topology T
template <Topology T>
struct DependencyIndicatorForIndexBuffer {
	static constexpr auto topology = T;
};

// We *have a dependency for an index buffer as needed (unknown topology)
struct DependencyEnforcerForIndexBuffer {};

// We *have a dependency for an index buffer for topology T
template <Topology T>
struct DependencyForIndexBuffer {
	static constexpr auto topology = T;
};

// We are granted a handle for ref
template <auto &ref>
struct Resolvant {
	static constexpr auto &handle = ref;
};

// We are granted a handle for an index buffer for topology T
template <Topology T>
struct ResolvantForIndexBuffer {
	static constexpr auto topology = T;
};

// We need all previous dependencies to be resolved
struct DependencySentinel {};

// We have synchronized ref
// TODO: this is useless now?
template <auto &ref, typename Phase>
struct Sync {
	static constexpr auto &handle = ref;
	using phase = Phase;
};

// Phase transition effect for ref
template <auto &ref, typename SrcPhase, typename DstPhase>
struct BarrierEffect {
	static constexpr auto &handle = ref;
	using src_phase = SrcPhase;
	using dst_phase = DstPhase;
};

// Render target effects
template <auto &ref>
struct TargetWrite {
	static constexpr auto &handle = ref;
};

template <auto &ref>
struct TargetRead {
	static constexpr auto &handle = ref;
};

// Dependency sequences for pipelines
template <auto &... refs>
consteval auto command_effects_for_streams(Tlist <contract <refs>...>)
{
	return Tlist <Dependency <refs>...> {};
}

template <typename ... Wrappers>
consteval auto command_effects_for_grcs(Tlist <Wrappers...>)
{
	return Tlist <Dependency <Wrappers::contract::handle>...> {};
}

template <Topology T, typename AS, typename GAMAP, typename GRCs>
consteval auto command_effects(const RasterizationPipeline <T, AS, GAMAP, GRCs> &pipeline)
{
	auto ib = Tlist <DependencyIndicatorForIndexBuffer <T>> {};
	auto as = command_effects_for_streams(AS());
	auto grcs = command_effects_for_grcs(GRCs());
	return tlist_concat(ib, as, grcs);
}

template <typename GAMAP, typename GRCs>
consteval auto command_effects(const ComputePipeline <GAMAP, GRCs> &pipeline)
{
	auto grcs = command_effects_for_grcs(GRCs());
	return grcs;
}

template <typename GAMAP, typename GRCs>
consteval auto command_effects(const MeshShadingPipeline <GAMAP, GRCs> &pipeline)
{
	auto grcs = command_effects_for_grcs(GRCs());
	return grcs;
}

template <typename GAMAP, typename GRCs>
consteval auto command_effects(const RayTracingPipeline <GAMAP, GRCs> &pipeline)
{
	auto grcs = command_effects_for_grcs(GRCs());
	return grcs;
}

} // namespace rcgp
