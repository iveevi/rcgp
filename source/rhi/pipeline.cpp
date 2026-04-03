#include <cstdlib>
#include <iostream>
#include <print>

#include "rhi/pipelines.hpp"

namespace rcgp {

vk::Pipeline compile_rasterization_pipeline(
	const Device &device,
	const RenderState &render_state,
	const vk::PrimitiveTopology topology,
	const vk::ShaderModule &vertex_shader_module,
	const std::optional <vk::ShaderModule> &fragment_shader_module,
	const char *vertex_entry,
	const char *fragment_entry,
	const vk::PipelineLayout &layout,
	const vk::ArrayProxy <vk::VertexInputBindingDescription> &vertex_bindings,
	const vk::ArrayProxy <vk::VertexInputAttributeDescription> &vertex_attributes,
	const RasterizationOptions &options
)
{
	bool dynamic_viewport_scissor = !options.extent.has_value();

	// Building the pipeline
	std::vector <vk::PipelineShaderStageCreateInfo> shader_stages;
	shader_stages.push_back(
		vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(vertex_shader_module)
			.setPName(vertex_entry)
	);
	if (fragment_shader_module.has_value()) {
		shader_stages.push_back(
			vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eFragment)
				.setModule(*fragment_shader_module)
				.setPName(fragment_entry)
		);
	}

	auto vertex_input = vk::PipelineVertexInputStateCreateInfo()
		.setVertexBindingDescriptions(vertex_bindings)
		.setVertexAttributeDescriptions(vertex_attributes);

	auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo()
		.setTopology(topology)
		.setPrimitiveRestartEnable(false);

	auto viewport_state = vk::PipelineViewportStateCreateInfo();
	auto viewport = vk::Viewport();
	auto scissor = vk::Rect2D();
	auto dynamic_states = std::array {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};
	auto dynamic_state = vk::PipelineDynamicStateCreateInfo()
		.setDynamicStates(dynamic_states);

	if (options.extent.has_value()) {
		auto extent = options.extent.value();
		viewport = vk::Viewport()
			.setX(0.0f)
			.setY(0.0f)
			.setWidth(float(extent.width))
			.setHeight(float(extent.height))
			.setMinDepth(0.0f)
			.setMaxDepth(1.0f);

		scissor = vk::Rect2D()
			.setOffset({ 0, 0 })
			.setExtent(extent);

		viewport_state
			.setViewports(viewport)
			.setScissors(scissor);
	} else {
		viewport_state
			.setViewportCount(1)
			.setScissorCount(1);
	}

	auto rasterization = vk::PipelineRasterizationStateCreateInfo()
		.setDepthClampEnable(false)
		.setRasterizerDiscardEnable(false)
		.setPolygonMode(options.polygon_mode)
		.setCullMode(options.cull_mode)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setDepthBiasEnable(false)
		.setLineWidth(options.line_width);

	auto multisampling = vk::PipelineMultisampleStateCreateInfo()
		.setRasterizationSamples(vk::SampleCountFlagBits::e1)
		.setSampleShadingEnable(false)
		.setMinSampleShading(1.0f);

	// Determine number of color attachments from the render state
	uint32_t color_att_count = 1;
	if (render_state.is <vk::PipelineRenderingCreateInfo> ())
		color_att_count = render_state.as <vk::PipelineRenderingCreateInfo> ().colorAttachmentCount;

	auto color_blend_attachment = vk::PipelineColorBlendAttachmentState()
		.setBlendEnable(options.alpha_blend)
		.setColorWriteMask(
			vk::ColorComponentFlagBits::eR
			| vk::ColorComponentFlagBits::eG
			| vk::ColorComponentFlagBits::eB
			| vk::ColorComponentFlagBits::eA
		);

	if (options.alpha_blend) {
		color_blend_attachment
			.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
			.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
			.setColorBlendOp(vk::BlendOp::eAdd)
			.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
			.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
			.setAlphaBlendOp(vk::BlendOp::eAdd);
	}

	std::vector <vk::PipelineColorBlendAttachmentState> color_blend_attachments;
	if (fragment_shader_module.has_value())
		color_blend_attachments.resize(color_att_count, color_blend_attachment);

	auto color_blend = vk::PipelineColorBlendStateCreateInfo()
		.setAttachments(color_blend_attachments);

	auto depth_stencil = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(options.depth_test)
		.setDepthWriteEnable(options.depth_test)
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setDepthBoundsTestEnable(false)
		.setStencilTestEnable(false);

	auto pipeline_info = vk::GraphicsPipelineCreateInfo()
		.setLayout(layout)
		.setPInputAssemblyState(&input_assembly)
		.setPVertexInputState(&vertex_input)
		.setPRasterizationState(&rasterization)
		.setPMultisampleState(&multisampling)
		.setPColorBlendState(&color_blend)
		.setPDepthStencilState(&depth_stencil)
		.setPViewportState(&viewport_state)
		.setStages(shader_stages)
		.setSubpass(0);

	if (dynamic_viewport_scissor)
		pipeline_info.setPDynamicState(&dynamic_state);

	if (render_state.is <vk::RenderPass> ())
		pipeline_info.setRenderPass(render_state.as <vk::RenderPass> ());
	else
		pipeline_info.setPNext(&render_state.as <vk::PipelineRenderingCreateInfo> ());

	auto [result, pipeline] = device.logical.createGraphicsPipeline(nullptr, pipeline_info, nullptr);
	if (result != vk::Result::eSuccess) {
		std::println(std::cerr, "failed to compile pipeline");
		std::abort();
	}

	return pipeline;
}

vk::Pipeline compile_compute_pipeline(
	const Device &device,
	const vk::ShaderModule &compute_shader_module,
	const char *compute_entry,
	const vk::PipelineLayout &layout
)
{
	auto stage = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eCompute)
		.setModule(compute_shader_module)
		.setPName(compute_entry);

	auto pipeline_info = vk::ComputePipelineCreateInfo()
		.setStage(stage)
		.setLayout(layout);

	auto [result, pipeline] = device.logical.createComputePipeline(nullptr, pipeline_info, nullptr);
	if (result != vk::Result::eSuccess) {
		std::println(std::cerr, "failed to compile pipeline");
		std::abort();
	}

	return pipeline;
}

vk::Pipeline compile_mesh_shading_pipeline(
	const Device &device,
	const RenderState &render_state,
	const vk::ShaderModule &task_shader_module,
	const vk::ShaderModule &mesh_shader_module,
	const vk::ShaderModule &fragment_shader_module,
	const char *task_entry,
	const char *mesh_entry,
	const char *fragment_entry,
	const vk::PipelineLayout &layout,
	const RasterizationOptions &options
)
{
	bool dynamic_viewport_scissor = !options.extent.has_value();

	auto shader_stages = std::array {
		vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eTaskEXT)
			.setModule(task_shader_module)
			.setPName(task_entry),
		vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eMeshEXT)
			.setModule(mesh_shader_module)
			.setPName(mesh_entry),
		vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(fragment_shader_module)
			.setPName(fragment_entry),
	};

	auto viewport_state = vk::PipelineViewportStateCreateInfo();
	auto viewport = vk::Viewport();
	auto scissor = vk::Rect2D();
	auto dynamic_states = std::array {
		vk::DynamicState::eViewport,
		vk::DynamicState::eScissor,
	};
	auto dynamic_state = vk::PipelineDynamicStateCreateInfo()
		.setDynamicStates(dynamic_states);

	if (options.extent.has_value()) {
		auto extent = options.extent.value();
		viewport = vk::Viewport()
			.setX(0.0f)
			.setY(0.0f)
			.setWidth(float(extent.width))
			.setHeight(float(extent.height))
			.setMinDepth(0.0f)
			.setMaxDepth(1.0f);

		scissor = vk::Rect2D()
			.setOffset({ 0, 0 })
			.setExtent(extent);

		viewport_state
			.setViewports(viewport)
			.setScissors(scissor);
	} else {
		viewport_state
			.setViewportCount(1)
			.setScissorCount(1);
	}

	auto rasterization = vk::PipelineRasterizationStateCreateInfo()
		.setDepthClampEnable(false)
		.setRasterizerDiscardEnable(false)
		.setPolygonMode(options.polygon_mode)
		.setCullMode(options.cull_mode)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setDepthBiasEnable(false)
		.setLineWidth(options.line_width);

	auto multisampling = vk::PipelineMultisampleStateCreateInfo()
		.setRasterizationSamples(vk::SampleCountFlagBits::e1)
		.setSampleShadingEnable(false)
		.setMinSampleShading(1.0f);

	auto color_blend_attachment = vk::PipelineColorBlendAttachmentState()
		.setBlendEnable(false)
		.setColorWriteMask(
			vk::ColorComponentFlagBits::eR
			| vk::ColorComponentFlagBits::eG
			| vk::ColorComponentFlagBits::eB
			| vk::ColorComponentFlagBits::eA
		);

	auto color_blend = vk::PipelineColorBlendStateCreateInfo()
		.setAttachments(color_blend_attachment);

	auto depth_stencil = vk::PipelineDepthStencilStateCreateInfo()
		.setDepthTestEnable(options.depth_test)
		.setDepthWriteEnable(options.depth_test)
		.setDepthCompareOp(vk::CompareOp::eLess)
		.setDepthBoundsTestEnable(false)
		.setStencilTestEnable(false);

	auto vertex_input = vk::PipelineVertexInputStateCreateInfo();
	auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo()
		.setTopology(vk::PrimitiveTopology::eTriangleList)
		.setPrimitiveRestartEnable(false);

	auto pipeline_info = vk::GraphicsPipelineCreateInfo()
		.setLayout(layout)
		.setPInputAssemblyState(&input_assembly)
		.setPVertexInputState(&vertex_input)
		.setPRasterizationState(&rasterization)
		.setPMultisampleState(&multisampling)
		.setPColorBlendState(&color_blend)
		.setPDepthStencilState(&depth_stencil)
		.setPViewportState(&viewport_state)
		.setStages(shader_stages)
		.setSubpass(0);

	if (dynamic_viewport_scissor)
		pipeline_info.setPDynamicState(&dynamic_state);

	if (render_state.is <vk::RenderPass> ())
		pipeline_info.setRenderPass(render_state.as <vk::RenderPass> ());
	else
		pipeline_info.setPNext(&render_state.as <vk::PipelineRenderingCreateInfo> ());

	auto [result, pipeline] = device.logical.createGraphicsPipeline(nullptr, pipeline_info, nullptr);
	if (result != vk::Result::eSuccess) {
		std::println(std::cerr, "failed to compile mesh shading pipeline");
		std::abort();
	}

	return pipeline;
}

vk::Pipeline compile_raytracing_pipeline(
	const Device &device,
	const vk::ArrayProxy <vk::PipelineShaderStageCreateInfo> &stages,
	const vk::ArrayProxy <vk::RayTracingShaderGroupCreateInfoKHR> &groups,
	const vk::PipelineLayout &layout,
	const RaytracingOptions &options
)
{
	auto dynamic_states = std::array { vk::DynamicState::eRayTracingPipelineStackSizeKHR };
	auto dynamic_state  = vk::PipelineDynamicStateCreateInfo()
		.setDynamicStates(dynamic_states);

	auto pipeline_info = vk::RayTracingPipelineCreateInfoKHR()
		.setStages(stages)
		.setGroups(groups)
		.setMaxPipelineRayRecursionDepth(options.max_recursion_depth)
		.setLayout(layout);

	if (options.dynamic_stack_size)
		pipeline_info.setPDynamicState(&dynamic_state);

	auto [result, pipeline] = device.logical.createRayTracingPipelineKHR(
		nullptr, nullptr, pipeline_info, nullptr, device.loader);
	if (result != vk::Result::eSuccess) {
		std::println(std::cerr, "failed to compile raytracing pipeline");
		std::abort();
	}

	return pipeline;
}

} // namespace rcgp
