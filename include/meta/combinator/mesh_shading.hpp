#pragma once

#include "../../dsl/generators.hpp"
#include "../../rhi/pipelines.hpp"
#include "../../rhi/shader_compiler.hpp"
#include "../../util/timer.hpp"
#include "../collect_gvrs.hpp"
#include "../combinator/rasterization.hpp"
#include "../group_allocation.hpp"
#include "../implicit_context.hpp"
#include "../pipeline/mesh_shading.hpp"
#include "../shader_stage.hpp"

struct MeshShadingCombinator {
	const Device &device;
	const ShaderCompiler &compiler;
	const vk::RenderPass &render_pass;
	RasterizationOptions options;

	template <typename TRet, typename ... Ts, typename MRet, typename ... Ms, typename FRet, typename ... Fs>
	auto operator()(
		shader_stage <ShaderStage::eTask, TRet, Ts...> &task,
		shader_stage <ShaderStage::eMesh, MRet, Ms...> &mesh,
		shader_stage <ShaderStage::eFragment, FRet, Fs...> &fragment
	) const {
		TSCOPE("mesh shading combinator");

		using task_icontext = find_implicit_context <Ts...> ::type;
		using mesh_icontext = find_implicit_context <Ms...> ::type;
		using fragment_icontext = find_implicit_context <Fs...> ::type;

		auto gvrs0 = Tlist <> {};
		auto gvrs1 = add_gvr_from_implicit_context <ShaderStage::eTask> (gvrs0, task_icontext());
		auto gvrs2 = add_gvr_from_implicit_context <ShaderStage::eMesh> (gvrs1, mesh_icontext());
		auto gvrs = add_gvr_from_implicit_context <ShaderStage::eFragment> (gvrs2, fragment_icontext());

		auto descriptor_gvrs = descriptable_resources_t <decltype(gvrs)> {};
		auto push_constant_gvrs = push_constant_resources_t <decltype(gvrs)> {};

		auto alloc = sequence_to_group_allocation(descriptor_gvrs);
		auto gamap = new_group_allocation_map(alloc);
		task->apply_group_allocation_map(gamap);
		mesh->apply_group_allocation_map(gamap);
		fragment->apply_group_allocation_map(gamap);

		auto pcmap = reference_sequence_to_push_constant_allocation_map(push_constant_gvrs);
		task->apply_push_constant_allocation_map(pcmap);
		mesh->apply_push_constant_allocation_map(pcmap);
		fragment->apply_push_constant_allocation_map(pcmap);

		auto tshader = generate_glsl(task);
		auto mshader = generate_glsl(mesh);
		auto fshader = generate_glsl(fragment);

		auto tspv = compiler.glsl_to_spirv(tshader, EShLangTask);
		auto mspv = compiler.glsl_to_spirv(mshader, EShLangMesh);
		auto fspv = compiler.glsl_to_spirv(fshader, EShLangFragment);

		auto task_shader_module = compiler.spirv_to_shader_module(tspv);
		auto mesh_shader_module = compiler.spirv_to_shader_module(mspv);
		auto fragment_shader_module = compiler.spirv_to_shader_module(fspv);

		auto dsls = reference_sequence_to_descriptor_set_layouts(device, descriptor_gvrs);
		auto pcrs = reference_sequence_to_push_constant_ranges(push_constant_gvrs);

		auto layout_info = vk::PipelineLayoutCreateInfo().setSetLayouts(dsls);
		if constexpr (push_constant_gvrs.size > 0)
			layout_info.setPushConstantRanges(pcrs);
		auto layout = device.logical.createPipelineLayout(layout_info);

		auto pipeline = compile_mesh_shading_pipeline(
			device,
			render_pass,
			task_shader_module,
			mesh_shader_module,
			fragment_shader_module,
			"main",
			"main",
			"main",
			layout,
			options
		);

		return MeshShadingPipeline <
			decltype(alloc),
			decltype(gvrs)
		> {
			.device = device.logical,
			.handle = pipeline,
			.layout = layout,
			.dsls = dsls,
		};
	}
};
