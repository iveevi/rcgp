#pragma once

#include "../dsl/generators.hpp"
#include "../rhi/device.hpp"
#include "../rhi/pipelines.hpp"
#include "../rhi/shader_compiler.hpp"
#include "../rhi/shader_compiler.hpp"
#include "../util/timer.hpp"
#include "input_assembly.hpp"
#include "pipelines.hpp"
#include "process_gvrs.hpp"
#include "resources_collect.hpp"
#include "shader_stage.hpp"

namespace rcgp {

struct DebugOptions {
	bool dump_assembly = false;
	bool dump_glsl = false;
};

template <typename ... Stage>
auto shaders_to_modules(
	const Device &device,
	const ShaderCompiler &compiler,
	const DebugOptions &debug,
	Stage &&... shaders
)
{
	auto process = [&](auto shader) {
		constexpr auto stage = decltype(shader)::stage;

		if (debug.dump_assembly) {
			auto sasm = generate_assembly(shader);
			printf("assembly:\n%s\n", sasm.c_str());
		}

		auto glsl = generate_glsl(shader);
		if (debug.dump_glsl)
			printf("glsl:\n%s\n", glsl.c_str());
		
		auto spirv = compiler.glsl_to_spirv(glsl, stage_to_esh(stage));

		return device.new_shader_module(spirv);
	};

	return std::tuple(process(shaders)...);
}

template <Topology T>
struct RasterizationCombinator {
	const Device &device;
	const ShaderCompiler &compiler;
	const vk::RenderPass &render_pass;
	RasterizationOptions options;
	DebugOptions debug;

	template <typename VertexShader, typename FragmentShader>
	requires is_vertex_shader_v <VertexShader>
		and is_fragment_shader_v <FragmentShader>
	auto operator()(VertexShader &vertex_shader, FragmentShader &fragment_shader) const {
		TSCOPE("rasterization combinator");

		// TODO: return a Tlist of error static strings...
		// probably needs a error <auto s>
		// [[maybe_unused]] constexpr bool interface_ok =
		// 	vertex_fragment_interface <VRet, Bs...> ::value;

		// TODO: we can get rid of these...
		using vertex_icontext = VertexShader::icontext;
		using fragment_icontext = FragmentShader::icontext;

		// Collect vertex attribute streams
		auto streams = collect_streams(vertex_icontext());

		// Generate vertex input bindings and attributes
		auto vertex_bindings = sequence_to_vertex_bindings(streams);
		auto vertex_attributes = sequence_to_vertex_attributes(streams);

		// Collect global resources
		auto gvrs = merge_stage_wrappers(tlist_concat(
			collect_gvrs <ShaderStage::eVertex> (vertex_icontext()),
			collect_gvrs <ShaderStage::eFragment> (fragment_icontext())
		));

		auto [layout, dsls, gamap] = apply_gvrs(
			device, gvrs,
			vertex_shader, fragment_shader
		);

		auto [vmod, fmod] = shaders_to_modules(
			device, compiler, debug,
			vertex_shader, fragment_shader
		);

		auto pipeline = compile_rasterization_pipeline(
			device,
			render_pass,
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
			decltype(gamap),
			decltype(gvrs)
		> (device.logical, pipeline, layout, dsls);
	}
};

template <Topology T>
struct DynamicRasterizationCombinator {
	const Device &device;
	const ShaderCompiler &compiler;
	RasterizationRenderingFormats formats;
	RasterizationOptions options;
	DebugOptions debug;

	template <typename VertexShader, typename FragmentShader>
	requires is_vertex_shader_v <VertexShader>
		and is_fragment_shader_v <FragmentShader>
	auto operator()(VertexShader &vertex_shader, FragmentShader &fragment_shader) const {
		TSCOPE("dynamic rasterization combinator");

		// [[maybe_unused]] constexpr bool interface_ok =
		// 	vertex_fragment_interface <VRet, Bs...> ::value;

		using vertex_icontext = VertexShader::icontext;
		using fragment_icontext = FragmentShader::icontext;

		auto streams = collect_streams(vertex_icontext());
		auto vertex_bindings = sequence_to_vertex_bindings(streams);
		auto vertex_attributes = sequence_to_vertex_attributes(streams);

		auto gvrs = merge_stage_wrappers(tlist_concat(
			collect_gvrs <ShaderStage::eVertex> (vertex_icontext()),
			collect_gvrs <ShaderStage::eFragment> (fragment_icontext())
		));

		auto [layout, dsls, gamap] = apply_gvrs(
			device, gvrs,
			vertex_shader,
			fragment_shader
		);

		auto [vmod, fmod] = shaders_to_modules(
			device, compiler, debug,
			vertex_shader, fragment_shader
		);

		auto pipeline = compile_rasterization_pipeline_dynamic(
			device,
			formats,
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
			decltype(gamap),
			decltype(gvrs)
		> (device.logical, pipeline, layout, dsls);
	}
};

struct ComputeCombinator {
	const Device &device;
	const ShaderCompiler &compiler;
	DebugOptions debug;

	template <typename Ret, typename ... As>
	auto operator()(shader_stage <ShaderStage::eCompute, Ret, As...> &compute) const {
		TSCOPE("compute combinator");

		using icontext = icontext_from_args_t <As...>;

		auto gvrs = collect_gvrs <ShaderStage::eCompute> (icontext());
		auto [layout, dsls, gamap] = apply_gvrs(device, gvrs, compute);
		
		auto [cmod] = shaders_to_modules(device, compiler, debug, compute);

		auto pipeline = compile_compute_pipeline(
			device,
			cmod,
			"main",
			layout
		);

		return ComputePipeline <
			decltype(gamap),
			decltype(gvrs)
		> (device.logical, pipeline, layout, dsls);
	}
};

struct MeshShadingCombinator {
	const Device &device;
	const ShaderCompiler &compiler;
	const vk::RenderPass &render_pass;
	RasterizationOptions options;
	DebugOptions debug;

	template <
		typename TRet, typename ... Ts,
		typename MRet, typename ... Ms,
		typename FRet, typename ... Fs
	>
	auto operator()(
		shader_stage <ShaderStage::eTask, TRet, Ts...> &task,
		shader_stage <ShaderStage::eMesh, MRet, Ms...> &mesh,
		shader_stage <ShaderStage::eFragment, FRet, Fs...> &fragment
	) const {
		TSCOPE("mesh shading combinator");

		using task_icontext = icontext_from_args_t <Ts...>;
		using mesh_icontext = icontext_from_args_t <Ms...>;
		using fragment_icontext = icontext_from_args_t <Fs...>;

		auto gvrs = merge_stage_wrappers(tlist_concat(
			collect_gvrs <ShaderStage::eTask> (task_icontext()),
			collect_gvrs <ShaderStage::eMesh> (mesh_icontext()),
			collect_gvrs <ShaderStage::eFragment> (fragment_icontext())
		));

		auto [layout, dsls, gamap] = apply_gvrs(device, gvrs, task, mesh, fragment);

		auto [tmod, mmod, fmod] = shaders_to_modules(
			device, compiler, debug,
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
			decltype(gamap),
			decltype(gvrs)
		> (device.logical, pipeline, layout, dsls);
	}
};

} // namespace rcgp
