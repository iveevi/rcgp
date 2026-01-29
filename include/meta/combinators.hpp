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
#include "stage_interface.hpp"

namespace rcgp {

template <typename ... Stage>
auto shaders_to_modules(const Device &device, const ShaderCompiler &compiler, Stage &&... shaders)
{
	auto process = [&](auto shader) {
		auto glsl = generate_glsl(shader);
		auto spirv = compiler.glsl_to_spirv(glsl, stage_to_esh(decltype(shader)::stage));
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

	template <
		typename VRet, typename ... As,
		typename FRet, typename ... Bs
	>
	auto operator()(
		shader_stage <ShaderStage::eVertex, VRet, As...> &vertex,
		shader_stage <ShaderStage::eFragment, FRet, Bs...> &fragment
	) const {
		TSCOPE("rasterization combinator");

		[[maybe_unused]] constexpr bool interface_ok =
			vertex_fragment_interface <VRet, Bs...> ::value;

		using vertex_icontext = icontext_from_args_t <As...>;
		using fragment_icontext = icontext_from_args_t <Bs...>;

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

		auto [layout, dsls, gamap] = apply_gvrs(device, gvrs, vertex, fragment);

		auto [vmod, fmod] = shaders_to_modules(
			device, compiler,
			vertex, fragment
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

struct ComputeCombinator {
	const Device &device;
	const ShaderCompiler &compiler;

	template <typename Ret, typename ... As>
	auto operator()(shader_stage <ShaderStage::eCompute, Ret, As...> &compute) const {
		TSCOPE("compute combinator");

		using icontext = icontext_from_args_t <As...>;

		auto gvrs = collect_gvrs <ShaderStage::eCompute> (icontext());
		auto [layout, dsls, gamap] = apply_gvrs(device, gvrs, compute);
		
		auto [cmod] = shaders_to_modules(device, compiler, compute);

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
			device, compiler,
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
