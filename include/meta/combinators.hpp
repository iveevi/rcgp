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
	DebugOptions debug;

	template <typename VertexShader, typename FragmentShader>
	requires is_vertex_shader_v <VertexShader>
		and is_fragment_shader_v <FragmentShader>
	auto operator()(VertexShader &vertex_shader, FragmentShader &fragment_shader) const {
		TSCOPE("rasterization combinator");

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
			decltype(gamap),
			decltype(gvrs)
		> (device.logical, pipeline, layout, dsls);
	}
};

struct ComputeCombinator {
	const Device &device;
	const ShaderCompiler &compiler;
	DebugOptions debug;

	template <typename ComputeShader>
	requires is_compute_shader_v <ComputeShader>
	auto operator()(ComputeShader &compute_shader) const {
		TSCOPE("compute combinator");

		auto gvrs = ComputeShader::gvrs;
		auto [layout, dsls, gamap] = apply_gvrs(device, gvrs, compute_shader);
		
		auto [cmod] = shaders_to_modules(device, compiler, debug, compute_shader);

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
	VkRenderPass render_pass = VK_NULL_HANDLE;
	RasterizationOptions options;
	DebugOptions debug;

	template <typename TaskShader, typename MeshShader, typename FragmentShader>
	auto operator()(TaskShader &task, MeshShader &mesh, FragmentShader &fragment) const {
		TSCOPE("mesh shading combinator");

		auto gvrs = merge_stage_wrappers(tlist_concat(
			TaskShader::gvrs,
			MeshShader::gvrs,
			FragmentShader::gvrs
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
