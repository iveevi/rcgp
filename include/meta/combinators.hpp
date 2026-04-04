#pragma once

#include <ranges>

#include "../dsl/generators.hpp"
#include "../dsl/optimization.hpp"
#include "../rhi/device.hpp"
#include "../rhi/pipelines.hpp"
#include "../rhi/shader_compiler.hpp"
#include "input_assembly.hpp"
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

		auto glsl = generate_glsl(shader);
		if (compiler_options.dump_glsl)
			printf("glsl:\n%s\n", glsl.c_str());
		
		auto spirv = compiler.glsl_to_spirv(glsl, stage);

		return device.new_shader_module(spirv);
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
	requires is_vertex_shader_v <VertexShader>
		and is_fragment_shader_v <FragmentShader>
	auto operator()(VertexShader &vertex_shader, FragmentShader &fragment_shader) const {
		// TODO: return a Tlist of error static strings?
		// probably needs a error <auto s>
		// [[maybe_unused]] constexpr bool interface_ok =
		// 	vertex_fragment_interface <VRet, Bs...> ::value;

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
	requires is_vertex_shader_v <VertexShader>
	auto operator()(VertexShader &vertex_shader) const {
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
	requires is_compute_shader_v <ComputeShader>
	auto operator()(ComputeShader &compute_shader) const {
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
