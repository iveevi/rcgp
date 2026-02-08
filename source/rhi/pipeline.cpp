#include <cstdlib>
#include <iostream>
#include <print>

#include "rhi/pipelines.hpp"
#include "util/timer.hpp"

namespace rcgp {

vk::Pipeline compile_rasterization_pipeline(
	const Device &device,
	const RenderState &render_state,
	const vk::PrimitiveTopology topology,
	const vk::ShaderModule &vertex_shader_module,
	const vk::ShaderModule &fragment_shader_module,
	const char *vertex_entry,
	const char *fragment_entry,
	const vk::PipelineLayout &layout,
	const vk::ArrayProxy <vk::VertexInputBindingDescription> &vertex_bindings,
	const vk::ArrayProxy <vk::VertexInputAttributeDescription> &vertex_attributes,
	const RasterizationOptions &options
)
{
	TSCOPE("compile rasterization pipeline");

	// Building the pipeline
	auto shader_stages = std::array<vk::PipelineShaderStageCreateInfo, 2> {};
	shader_stages[0].stage = vk::ShaderStageFlagBits::eVertex;
	shader_stages[0].module = vertex_shader_module;
	shader_stages[0].pName = vertex_entry;
	shader_stages[1].stage = vk::ShaderStageFlagBits::eFragment;
	shader_stages[1].module = fragment_shader_module;
	shader_stages[1].pName = fragment_entry;

	vk::PipelineVertexInputStateCreateInfo vertex_input {};
	vertex_input.vertexBindingDescriptionCount = vertex_bindings.size();
	vertex_input.pVertexBindingDescriptions = vertex_bindings.data();
	vertex_input.vertexAttributeDescriptionCount = vertex_attributes.size();
	vertex_input.pVertexAttributeDescriptions = vertex_attributes.data();

	vk::PipelineInputAssemblyStateCreateInfo input_assembly {};
	input_assembly.topology = topology;
	input_assembly.primitiveRestartEnable = false;

	vk::Viewport viewport {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = float(options.extent.width);
	viewport.height = float(options.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = options.extent;

	vk::PipelineViewportStateCreateInfo viewport_state {};
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterization {};
	rasterization.depthClampEnable = false;
	rasterization.rasterizerDiscardEnable = false;
	rasterization.polygonMode = options.polygon_mode;
	rasterization.cullMode = options.cull_mode;
	rasterization.frontFace = vk::FrontFace::eCounterClockwise;
	rasterization.depthBiasEnable = false;
	rasterization.lineWidth = 1.0f;

	vk::PipelineMultisampleStateCreateInfo multisampling {};
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	multisampling.sampleShadingEnable = false;
	multisampling.minSampleShading = 1.0f;

	vk::PipelineColorBlendAttachmentState color_blend_attachment {};
	color_blend_attachment.blendEnable = options.alpha_blend;
	color_blend_attachment.colorWriteMask =
		vk::ColorComponentFlagBits::eR
		| vk::ColorComponentFlagBits::eG
		| vk::ColorComponentFlagBits::eB
		| vk::ColorComponentFlagBits::eA;

	if (options.alpha_blend) {
		color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
		color_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		color_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;
		color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
		color_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
		color_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;
	}

	vk::PipelineColorBlendStateCreateInfo color_blend {};
	color_blend.attachmentCount = 1;
	color_blend.pAttachments = &color_blend_attachment;

	vk::PipelineDepthStencilStateCreateInfo depth_stencil {};
	depth_stencil.depthTestEnable = options.depth_test;
	depth_stencil.depthWriteEnable = options.depth_test;
	depth_stencil.depthCompareOp = vk::CompareOp::eLess;
	depth_stencil.depthBoundsTestEnable = false;
	depth_stencil.stencilTestEnable = false;

	vk::GraphicsPipelineCreateInfo pipeline_info {};
	pipeline_info.layout = layout;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pVertexInputState = &vertex_input;
	pipeline_info.pRasterizationState = &rasterization;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pColorBlendState = &color_blend;
	pipeline_info.pDepthStencilState = &depth_stencil;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.stageCount = shader_stages.size();
	pipeline_info.pStages = shader_stages.data();
	pipeline_info.subpass = 0;

	if (render_state.is <vk::RenderPass> ()) {
		pipeline_info.renderPass = render_state.as <vk::RenderPass> ();
	} else {
		pipeline_info.pNext = &render_state.as <vk::PipelineRenderingCreateInfo> ();
	}

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
	TSCOPE("compile compute pipeline");

	vk::PipelineShaderStageCreateInfo stage {};
	stage.stage = vk::ShaderStageFlagBits::eCompute;
	stage.module = compute_shader_module;
	stage.pName = compute_entry;

	vk::ComputePipelineCreateInfo pipeline_info {};
	pipeline_info.stage = stage;
	pipeline_info.layout = layout;

	auto [result, pipeline] = device.logical.createComputePipeline(nullptr, pipeline_info, nullptr);
	if (result != vk::Result::eSuccess) {
		std::println(std::cerr, "failed to compile pipeline");
		std::abort();
	}

	return pipeline;
}

vk::Pipeline compile_mesh_shading_pipeline(
	const Device &device,
	const vk::RenderPass &render_pass,
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
	TSCOPE("compile mesh shading pipeline");

	auto shader_stages = std::array<vk::PipelineShaderStageCreateInfo, 3> {};
	shader_stages[0].stage = vk::ShaderStageFlagBits::eTaskEXT;
	shader_stages[0].module = task_shader_module;
	shader_stages[0].pName = task_entry;
	shader_stages[1].stage = vk::ShaderStageFlagBits::eMeshEXT;
	shader_stages[1].module = mesh_shader_module;
	shader_stages[1].pName = mesh_entry;
	shader_stages[2].stage = vk::ShaderStageFlagBits::eFragment;
	shader_stages[2].module = fragment_shader_module;
	shader_stages[2].pName = fragment_entry;

	vk::Viewport viewport {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = float(options.extent.width);
	viewport.height = float(options.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vk::Rect2D scissor {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = options.extent;

	vk::PipelineViewportStateCreateInfo viewport_state {};
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	vk::PipelineRasterizationStateCreateInfo rasterization {};
	rasterization.depthClampEnable = false;
	rasterization.rasterizerDiscardEnable = false;
	rasterization.polygonMode = options.polygon_mode;
	rasterization.cullMode = options.cull_mode;
	rasterization.frontFace = vk::FrontFace::eCounterClockwise;
	rasterization.depthBiasEnable = false;
	rasterization.lineWidth = 1.0f;

	vk::PipelineMultisampleStateCreateInfo multisampling {};
	multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
	multisampling.sampleShadingEnable = false;
	multisampling.minSampleShading = 1.0f;

	vk::PipelineColorBlendAttachmentState color_blend_attachment {};
	color_blend_attachment.blendEnable = false;
	color_blend_attachment.colorWriteMask =
		vk::ColorComponentFlagBits::eR
		| vk::ColorComponentFlagBits::eG
		| vk::ColorComponentFlagBits::eB
		| vk::ColorComponentFlagBits::eA;

	vk::PipelineColorBlendStateCreateInfo color_blend {};
	color_blend.attachmentCount = 1;
	color_blend.pAttachments = &color_blend_attachment;

	vk::PipelineDepthStencilStateCreateInfo depth_stencil {};
	depth_stencil.depthTestEnable = options.depth_test;
	depth_stencil.depthWriteEnable = options.depth_test;
	depth_stencil.depthCompareOp = vk::CompareOp::eLess;
	depth_stencil.depthBoundsTestEnable = false;
	depth_stencil.stencilTestEnable = false;

	vk::PipelineVertexInputStateCreateInfo vertex_input {};
	vk::PipelineInputAssemblyStateCreateInfo input_assembly {};
	input_assembly.topology = vk::PrimitiveTopology::eTriangleList;
	input_assembly.primitiveRestartEnable = false;

	vk::GraphicsPipelineCreateInfo pipeline_info {};
	pipeline_info.layout = layout;
	pipeline_info.pInputAssemblyState = &input_assembly;
	pipeline_info.pVertexInputState = &vertex_input;
	pipeline_info.pRasterizationState = &rasterization;
	pipeline_info.pMultisampleState = &multisampling;
	pipeline_info.pColorBlendState = &color_blend;
	pipeline_info.pDepthStencilState = &depth_stencil;
	pipeline_info.pViewportState = &viewport_state;
	pipeline_info.stageCount = shader_stages.size();
	pipeline_info.pStages = shader_stages.data();
	pipeline_info.renderPass = render_pass;
	pipeline_info.subpass = 0;

	auto [result, pipeline] = device.logical.createGraphicsPipeline(nullptr, pipeline_info, nullptr);
	if (result != vk::Result::eSuccess) {
		std::println(std::cerr, "failed to compile mesh shading pipeline");
		std::abort();
	}

	return pipeline;
}

} // namespace rcgp
