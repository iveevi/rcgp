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
	const RenderState &render_state;
	RasterizationOptions options;
	CompilerOptions compiler_options;

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
	const vk::RenderPass &render_pass;
	RasterizationOptions options;
	CompilerOptions compiler_options;

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
			render_pass,
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

// Macros for the resulting types
#define $rasterization_pipeline_t(T, vs, fs) \
	decltype(std::declval <RasterizationCombinator <Topology::T>> ()(vs, fs))

#define $compute_pipeline_t(T, cs) \
	decltype(std::declval <ComputeCombinator> ()(cs))

#define $mesh_shader_pipeline_t(T, ts, ms, fs) \
	decltype(std::declval <MeshShadingCombinator> ()(ts, ms, fs))

} // namespace rcgp
