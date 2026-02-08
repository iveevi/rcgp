#include <cstdlib>
#include <iostream>
#include <print>

#include "rhi/pipelines.hpp"

namespace rcgp {

VkPipeline compile_rasterization_pipeline(
	const Device &device,
	const RenderState &render_state,
	VkPrimitiveTopology topology,
	VkShaderModule vertex_shader_module,
	VkShaderModule fragment_shader_module,
	const char *vertex_entry,
	const char *fragment_entry,
	VkPipelineLayout layout,
	const std::span <const VkVertexInputBindingDescription> &vertex_bindings,
	const std::span <const VkVertexInputAttributeDescription> &vertex_attributes,
	const RasterizationOptions &options
)
{

	auto shader_stages = std::array <VkPipelineShaderStageCreateInfo, 2> {};
	shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shader_stages[0].module = vertex_shader_module;
	shader_stages[0].pName = vertex_entry;
	shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stages[1].module = fragment_shader_module;
	shader_stages[1].pName = fragment_entry;

	VkPipelineVertexInputStateCreateInfo vertex_input {};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertex_input.vertexBindingDescriptionCount = vertex_bindings.size();
	vertex_input.pVertexBindingDescriptions = vertex_bindings.data();
	vertex_input.vertexAttributeDescriptionCount = vertex_attributes.size();
	vertex_input.pVertexAttributeDescriptions = vertex_attributes.data();

	VkPipelineInputAssemblyStateCreateInfo input_assembly {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = topology;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkViewport viewport {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = float(options.extent.width);
	viewport.height = float(options.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = options.extent;

	VkPipelineViewportStateCreateInfo viewport_state {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterization {};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.depthClampEnable = VK_FALSE;
	rasterization.rasterizerDiscardEnable = VK_FALSE;
	rasterization.polygonMode = options.polygon_mode;
	rasterization.cullMode = options.cull_mode;
	rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization.depthBiasEnable = VK_FALSE;
	rasterization.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampling {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.minSampleShading = 1.0f;

	VkPipelineColorBlendAttachmentState color_blend_attachment {};
	color_blend_attachment.blendEnable = options.alpha_blend;
	color_blend_attachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT
		| VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT
		| VK_COLOR_COMPONENT_A_BIT;

	if (options.alpha_blend) {
		color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
	}

	VkPipelineColorBlendStateCreateInfo color_blend {};
	color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend.attachmentCount = 1;
	color_blend.pAttachments = &color_blend_attachment;

	VkPipelineDepthStencilStateCreateInfo depth_stencil {};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable = options.depth_test;
	depth_stencil.depthWriteEnable = options.depth_test;
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.stencilTestEnable = VK_FALSE;

	VkGraphicsPipelineCreateInfo pipeline_info {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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

	if (render_state.is <VkRenderPass> ()) {
		pipeline_info.renderPass = render_state.as <VkRenderPass> ();
	} else {
		pipeline_info.pNext = &render_state.as <VkPipelineRenderingCreateInfo> ();
	}

	auto pipeline = VkPipeline(VK_NULL_HANDLE);
	auto result = vkCreateGraphicsPipelines(
		device.logical,
		VK_NULL_HANDLE,
		1,
		&pipeline_info,
		nullptr,
		&pipeline
	);
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed to compile pipeline");
		std::abort();
	}

	return pipeline;
}

VkPipeline compile_compute_pipeline(
	const Device &device,
	VkShaderModule compute_shader_module,
	const char *compute_entry,
	VkPipelineLayout layout
)
{

	VkPipelineShaderStageCreateInfo stage {};
	stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage.module = compute_shader_module;
	stage.pName = compute_entry;

	VkComputePipelineCreateInfo pipeline_info {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipeline_info.stage = stage;
	pipeline_info.layout = layout;

	auto pipeline = VkPipeline(VK_NULL_HANDLE);
	auto result = vkCreateComputePipelines(
		device.logical,
		VK_NULL_HANDLE,
		1,
		&pipeline_info,
		nullptr,
		&pipeline
	);
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed to compile pipeline");
		std::abort();
	}

	return pipeline;
}

VkPipeline compile_mesh_shading_pipeline(
	const Device &device,
	VkRenderPass render_pass,
	VkShaderModule task_shader_module,
	VkShaderModule mesh_shader_module,
	VkShaderModule fragment_shader_module,
	const char *task_entry,
	const char *mesh_entry,
	const char *fragment_entry,
	VkPipelineLayout layout,
	const RasterizationOptions &options
)
{

	auto shader_stages = std::array <VkPipelineShaderStageCreateInfo, 3> {};
	shader_stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[0].stage = VK_SHADER_STAGE_TASK_BIT_EXT;
	shader_stages[0].module = task_shader_module;
	shader_stages[0].pName = task_entry;
	shader_stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[1].stage = VK_SHADER_STAGE_MESH_BIT_EXT;
	shader_stages[1].module = mesh_shader_module;
	shader_stages[1].pName = mesh_entry;
	shader_stages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shader_stages[2].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shader_stages[2].module = fragment_shader_module;
	shader_stages[2].pName = fragment_entry;

	VkViewport viewport {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = float(options.extent.width);
	viewport.height = float(options.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent = options.extent;

	VkPipelineViewportStateCreateInfo viewport_state {};
	viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_state.viewportCount = 1;
	viewport_state.pViewports = &viewport;
	viewport_state.scissorCount = 1;
	viewport_state.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterization {};
	rasterization.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterization.depthClampEnable = VK_FALSE;
	rasterization.rasterizerDiscardEnable = VK_FALSE;
	rasterization.polygonMode = options.polygon_mode;
	rasterization.cullMode = options.cull_mode;
	rasterization.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterization.depthBiasEnable = VK_FALSE;
	rasterization.lineWidth = 1.0f;

	VkPipelineMultisampleStateCreateInfo multisampling {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.minSampleShading = 1.0f;

	VkPipelineColorBlendAttachmentState color_blend_attachment {};
	color_blend_attachment.blendEnable = VK_FALSE;
	color_blend_attachment.colorWriteMask =
		VK_COLOR_COMPONENT_R_BIT
		| VK_COLOR_COMPONENT_G_BIT
		| VK_COLOR_COMPONENT_B_BIT
		| VK_COLOR_COMPONENT_A_BIT;

	VkPipelineColorBlendStateCreateInfo color_blend {};
	color_blend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blend.attachmentCount = 1;
	color_blend.pAttachments = &color_blend_attachment;

	VkPipelineDepthStencilStateCreateInfo depth_stencil {};
	depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depth_stencil.depthTestEnable = options.depth_test;
	depth_stencil.depthWriteEnable = options.depth_test;
	depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depth_stencil.depthBoundsTestEnable = VK_FALSE;
	depth_stencil.stencilTestEnable = VK_FALSE;

	VkPipelineVertexInputStateCreateInfo vertex_input {};
	vertex_input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	VkPipelineInputAssemblyStateCreateInfo input_assembly {};
	input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable = VK_FALSE;

	VkGraphicsPipelineCreateInfo pipeline_info {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
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

	auto pipeline = VkPipeline(VK_NULL_HANDLE);
	auto result = vkCreateGraphicsPipelines(
		device.logical,
		VK_NULL_HANDLE,
		1,
		&pipeline_info,
		nullptr,
		&pipeline
	);
	if (result != VK_SUCCESS) {
		std::println(std::cerr, "failed to compile mesh shading pipeline");
		std::abort();
	}

	return pipeline;
}

} // namespace rcgp
