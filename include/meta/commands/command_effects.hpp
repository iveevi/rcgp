#pragma once

#include "../pipeline/compute.hpp"
#include "../pipeline/mesh_shading.hpp"
#include "../pipeline/rasterization.hpp"

// We have a dependency on ref
template <auto &ref>
struct Dependency {};

// We *might have a dependency for an index buffer for topology T
template <Topology T>
struct DependencyIndicatorForIndexBuffer {};

// We *have a dependency for an index buffer as needed (unknown topology)
struct DependencyEnforcerForIndexBuffer {};

// We *have a dependency for an index buffer for topology T
template <Topology T>
struct DependencyForIndexBuffer {};

// We are granted a handle for ref
template <auto &ref>
struct Resolvant {};

// We are granted a handle for an index buffer for topology T
template <Topology T>
struct ResolvantForIndexBuffer {};

// We need all previous dependencies to be resolved
struct DependencySentinel {};

// We have synchronized ref
template <auto &ref, typename Phase>
struct Sync {};

// Dependency sequences for pipelines
template <auto &... refs>
consteval auto command_effects_for_streams(Tlist <reference <refs>...>)
{
	return Tlist <Dependency <refs>...> {};
}

template <typename ... Wrappers>
consteval auto command_effects_for_grcs(Tlist <Wrappers...>)
{
	return Tlist <Dependency <Wrappers::reference::handle>...> {};
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
