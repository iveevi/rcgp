#pragma once

#include <ranges>

#include "../dsl/generators.hpp"
#include "../dsl/optimization.hpp"
#include "../rhi/device.hpp"
#include "../rhi/pipelines.hpp"
#include "../rhi/shader_compiler.hpp"
#include "input_assembly.hpp"
#include "interface_validation.hpp"
#include "pipelines.hpp"
#include "process_gvrs.hpp"
#include "resolve_trace_groups.hpp"
#include "resources_collect.hpp"
#include "shader_stage.hpp"

namespace rcgp {

struct CompilerOptions {
	OptimizationPhases optimization_flags = OptimizationPhases::eNone;
	bool dump_assembly = false;
	bool dump_glsl = false;
};

template <typename VertexShader, typename FragmentShader>
void transfer_io_rates(const VertexShader &vertex_shader, const FragmentShader &fragment_shader)
{
	for (const auto &[i, sin] : std::views::enumerate(fragment_shader->stage_inputs)) {
		auto sout = vertex_shader->stage_outputs.at(i);
		sin.properties = sout.properties;
	}
}

template <typename ... Stage>
auto shaders_to_modules(
	const Device &device,
	const ShaderCompiler &compiler,
	const CompilerOptions &compiler_options,
	Stage &&... shaders
)
{
	auto process = [&](auto shader) {
		constexpr auto stage = decltype(shader)::stage;

		optimize(shader, compiler_options.optimization_flags);

		if (compiler_options.dump_assembly) {
			auto sasm = generate_assembly(shader);
			printf("assembly:\n%s\n", sasm.c_str());
		}

		auto artifacts = compiler.compile(shader, stage);

		if (compiler_options.dump_glsl) {
			if (artifacts.glsl.empty())
				artifacts.glsl = generate_glsl(shader);
			printf("glsl:\n%s\n", artifacts.glsl.c_str());
		}

		return device.new_shader_module(artifacts.spirv);
	};

	return std::tuple(process(shaders)...);
}

template <Topology T>
struct RasterizationCombinator {
	const Device &device;
	const ShaderCompiler &compiler;
	CompilerOptions compiler_options;
	const RenderState &render_state;
	RasterizationOptions options;

	template <typename VertexShader, typename FragmentShader>
	auto operator()(VertexShader &vertex_shader, FragmentShader &fragment_shader) const {
		constexpr bool is_vertex = is_vertex_shader_v <VertexShader>;
		static_assert(is_vertex, "rasterization combinator: arg@0 is not a vertex shader");
		constexpr bool is_fragment = is_fragment_shader_v <FragmentShader>;
		static_assert(is_fragment, "rasterization combinator: arg@1 is not a fragment shader");
		constexpr auto io = check_vertex_fragment_io <VertexShader, FragmentShader> ();
		if constexpr (not std::is_same_v <std::decay_t <decltype(io)>, int>)
			static_assert(false, io);

		transfer_io_rates(vertex_shader, fragment_shader);

		// Collect vertex attribute streams
		auto streams = collect_streams(typename VertexShader::icontext());

		// Generate vertex input bindings and attributes
		auto vertex_bindings = sequence_to_vertex_bindings(streams);
		auto vertex_attributes = sequence_to_vertex_attributes(streams);

		// Collect global resources
		auto gvrs = merge_stage_wrappers(tlist_concat(
			VertexShader::gvrs,
			FragmentShader::gvrs
		));

		auto [layout, dsls, grcs, gamap] = apply_gvrs(
			device, gvrs,
			vertex_shader, fragment_shader
		);

		auto [vmod, fmod] = shaders_to_modules(
			device, compiler, compiler_options,
			vertex_shader, fragment_shader
		);

		auto pipeline = compile_rasterization_pipeline(
			device,
			render_state,
			translate_topology(T),
			vmod, fmod,
			"main", "main",
			layout,
			vertex_bindings,
			vertex_attributes,
			options
		);

		return RasterizationPipeline <
			T,
			decltype(streams),
			decltype(grcs),
			decltype(gvrs)
		> (pipeline, layout, dsls, gamap);
	}

	template <typename VertexShader>
	auto operator()(VertexShader &vertex_shader) const {
		constexpr bool is_vertex = is_vertex_shader_v <VertexShader>;
		static_assert(is_vertex, "rasterization combinator: arg@0 is not a vertex shader");

		auto streams = collect_streams(typename VertexShader::icontext());

		auto vertex_bindings = sequence_to_vertex_bindings(streams);
		auto vertex_attributes = sequence_to_vertex_attributes(streams);

		auto gvrs = merge_stage_wrappers(VertexShader::gvrs);

		auto [layout, dsls, grcs, gamap] = apply_gvrs(device, gvrs, vertex_shader);

		auto [vmod] = shaders_to_modules(device, compiler, compiler_options, vertex_shader);

		auto pipeline = compile_rasterization_pipeline(
			device,
			render_state,
			translate_topology(T),
			vmod, std::nullopt,
			"main", nullptr,
			layout,
			vertex_bindings,
			vertex_attributes,
			options
		);

		return RasterizationPipeline <
			T,
			decltype(streams),
			decltype(grcs),
			decltype(gvrs)
		> (pipeline, layout, dsls, gamap);
	}
};

struct ComputeCombinator {
	const Device &device;
	const ShaderCompiler &compiler;
	CompilerOptions compiler_options;

	template <typename ComputeShader>
	auto operator()(ComputeShader &compute_shader) const {
		constexpr bool is_compute = is_compute_shader_v <ComputeShader>;
		static_assert(is_compute, "compute combinator: arg@0 is not a compute shader");

		auto gvrs = ComputeShader::gvrs;
		auto [layout, dsls, grcs, gamap] = apply_gvrs(device, gvrs, compute_shader);
		
		auto [cmod] = shaders_to_modules(device, compiler, compiler_options, compute_shader);

		auto pipeline = compile_compute_pipeline(
			device,
			cmod,
			"main",
			layout
		);

		return ComputePipeline <
			decltype(grcs),
			decltype(gvrs)
		> (pipeline, layout, dsls, gamap);
	}
};

struct MeshShadingCombinator {
	const Device &device;
	const ShaderCompiler &compiler;
	CompilerOptions compiler_options;
	const RenderState &render_state;
	RasterizationOptions options;

	template <typename TaskShader, typename MeshShader, typename FragmentShader>
	auto operator()(TaskShader &task, MeshShader &mesh, FragmentShader &fragment) const {
		constexpr bool is_task = is_task_shader_v <TaskShader>;
		static_assert(is_task, "mesh-shading combinator: arg@0 is not a task shader");
		constexpr bool is_mesh = is_mesh_shader_v <MeshShader>;
		static_assert(is_mesh, "mesh-shading combinator: arg@1 is not a mesh shader");
		constexpr bool is_fragment = is_fragment_shader_v <FragmentShader>;
		static_assert(is_fragment, "mesh-shading combinator: arg@2 is not a fragment shader");
		constexpr auto payload = check_task_mesh_payload <TaskShader, MeshShader> ();
		if constexpr (not std::is_same_v <std::decay_t <decltype(payload)>, int>)
			static_assert(false, payload);

		constexpr auto io = check_mesh_fragment_io <MeshShader, FragmentShader> ();
		if constexpr (not std::is_same_v <std::decay_t <decltype(io)>, int>)
			static_assert(false, io);

		auto gvrs = merge_stage_wrappers(tlist_concat(
			TaskShader::gvrs,
			MeshShader::gvrs,
			FragmentShader::gvrs
		));

		auto [layout, dsls, grcs, gamap] = apply_gvrs(device, gvrs, task, mesh, fragment);

		auto [tmod, mmod, fmod] = shaders_to_modules(
			device, compiler, compiler_options,
			task, mesh, fragment
		);

		auto pipeline = compile_mesh_shading_pipeline(
			device,
			render_state,
			tmod, mmod, fmod,
			"main", "main", "main",
			layout,
			options
		);

		return MeshShadingPipeline <
			decltype(grcs),
			decltype(gvrs)
		> (pipeline, layout, dsls, gamap);
	}
};

struct RayTracingCombinator {
	const Device &device;
	const ShaderCompiler &compiler;
	CompilerOptions compiler_options;
	// TODO: automate the handling SBT buffers
	RaytracingOptions options;

	template <
		typename RayGenerationShader,
		typename ... MissShaders,
		typename ... ClosestHitShaders
	>
	auto operator()(
		const RayGenerationShader &rgen,
		const std::tuple <MissShaders...> &misses,
		const std::tuple <ClosestHitShaders...> &chits
	) const {
		constexpr bool is_raygen = is_raygen_shader_v <RayGenerationShader>;
		static_assert(is_raygen, "ray tracing combinator: arg@0 is not a ray generation shader");
		constexpr bool all_miss = (is_miss_shader_v <MissShaders> and ...);
		static_assert(all_miss, "ray tracing combinator: arg@1 (miss shader tuple) contains a non-miss-shader element");
		constexpr bool all_chit = (is_closest_hit_shader_v <ClosestHitShaders> and ...);
		static_assert(all_chit, "ray tracing combinator: arg@2 (closest-hit shader tuple) contains a non-closest-hit-shader element");
		constexpr auto coverage = check_raytracing_coverage <RayGenerationShader> (
			Tlist <MissShaders...> {}, Tlist <ClosestHitShaders...> {});
		if constexpr (not std::is_same_v <std::decay_t <decltype(coverage)>, int>)
			static_assert(false, coverage);

		auto gvrs = [&] <size_t... CI, size_t... MI> (
			std::index_sequence <CI...>,
			std::index_sequence <MI...>
		) {
			return merge_stage_wrappers(tlist_concat(
				RayGenerationShader::gvrs,
				std::tuple_element_t <MI, std::tuple <MissShaders...>> ::gvrs...,
				std::tuple_element_t <CI, std::tuple <ClosestHitShaders...>> ::gvrs...
			));
		}(std::index_sequence_for <ClosestHitShaders...> {}, std::index_sequence_for <MissShaders...> {});

		auto [layout, dsls, grcs, gamap] = std::apply(
			[&](const auto &... chit_shaders) {
				return std::apply(
					[&](const auto &... miss_shaders) {
						return apply_gvrs(device, gvrs,
							rgen, miss_shaders..., chit_shaders...
						);
					}, misses
				);
			}, chits
		);

		// Resolve trace group SBT offsets and payload indices
		std::apply(
			[&](const auto &... chit_shaders) {
				resolve_trace_groups(rgen, misses, chit_shaders...);
			}, chits
		);

		auto modules = std::apply(
			[&](const auto &... chit_shaders) {
				return std::apply(
					[&](const auto &... miss_shaders) {
						return shaders_to_modules(device, compiler, compiler_options,
							rgen, miss_shaders..., chit_shaders...);
					},
					misses
				);
			},
			chits
		);

		std::vector <vk::PipelineShaderStageCreateInfo> stages;

		stages.push_back(vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eRaygenKHR)
			.setModule(std::get <0> (modules))
			.setPName("main"));

		constexpr_for(MI, sizeof...(MissShaders),
			(stages.push_back(vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eMissKHR)
				.setModule(std::get <1 + MI> (modules))
				.setPName("main")), ...)
		);

		constexpr_for(CI, sizeof...(ClosestHitShaders),
			(stages.push_back(vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eClosestHitKHR)
				.setModule(std::get <1 + sizeof...(MissShaders) + CI> (modules))
				.setPName("main")), ...)
		);

		// 5. Build shader groups (same ordering)
		std::vector <vk::RayTracingShaderGroupCreateInfoKHR> groups;

		groups.push_back(vk::RayTracingShaderGroupCreateInfoKHR()
			.setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
			.setGeneralShader(0)
			.setClosestHitShader(VK_SHADER_UNUSED_KHR)
			.setAnyHitShader(VK_SHADER_UNUSED_KHR)
			.setIntersectionShader(VK_SHADER_UNUSED_KHR));

		constexpr_for(MI, sizeof...(MissShaders),
			(groups.push_back(vk::RayTracingShaderGroupCreateInfoKHR()
				.setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
				.setGeneralShader(uint32_t(1 + MI))
				.setClosestHitShader(VK_SHADER_UNUSED_KHR)
				.setAnyHitShader(VK_SHADER_UNUSED_KHR)
				.setIntersectionShader(VK_SHADER_UNUSED_KHR)), ...)
		);

		constexpr_for(CI, sizeof...(ClosestHitShaders),
			(groups.push_back(vk::RayTracingShaderGroupCreateInfoKHR()
				.setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
				.setGeneralShader(VK_SHADER_UNUSED_KHR)
				.setClosestHitShader(uint32_t(1 + sizeof...(MissShaders) + CI))
				.setAnyHitShader(VK_SHADER_UNUSED_KHR)
				.setIntersectionShader(VK_SHADER_UNUSED_KHR)), ...)
		);

		// 6. Compile pipeline and return
		auto pipeline = compile_raytracing_pipeline(device, stages, groups, layout, options);

		return RayTracingPipeline <
			decltype(grcs),
			decltype(gvrs)
		> (pipeline, layout, dsls, gamap);
	}
};

} // namespace rcgp
