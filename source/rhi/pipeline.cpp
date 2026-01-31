#include "rhi/pipelines.hpp"
#include "util/logging.hpp"
#include "util/timer.hpp"

namespace rcgp {

vk::Pipeline compile_rasterization_pipeline(
	const Device &device,
	const vk::RenderPass &render_pass,
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
	auto shader_stages = std::array {
		vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(vertex_shader_module)
			.setPName(vertex_entry),
		vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(fragment_shader_module)
			.setPName(fragment_entry),
	};

	auto vertex_input = vk::PipelineVertexInputStateCreateInfo()
		.setVertexBindingDescriptions(vertex_bindings)
		.setVertexAttributeDescriptions(vertex_attributes);

	auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo()
		.setTopology(topology)
		.setPrimitiveRestartEnable(false);

	auto viewport = vk::Viewport()
		.setX(0.0f)
		.setY(0.0f)
		.setWidth(float(options.extent.width))
		.setHeight(float(options.extent.height))
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);

	auto scissor = vk::Rect2D()
		.setOffset({ 0, 0 })
		.setExtent(options.extent);

	auto viewport_state = vk::PipelineViewportStateCreateInfo()
		.setViewports(viewport)
		.setScissors(scissor);

	auto rasterization = vk::PipelineRasterizationStateCreateInfo()
		.setDepthClampEnable(false)
		.setRasterizerDiscardEnable(false)
		.setPolygonMode(options.polygon_mode)
		.setCullMode(options.cull_mode)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setDepthBiasEnable(false)
		.setLineWidth(1.0f);

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
		.setRenderPass(render_pass)
		.setSubpass(0);

	auto [result, pipeline] = device.logical.createGraphicsPipeline(nullptr, pipeline_info, nullptr);
	assertion(result == vk::Result::eSuccess, "failed to compile pipeline");

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

	auto stage = vk::PipelineShaderStageCreateInfo()
		.setStage(vk::ShaderStageFlagBits::eCompute)
		.setModule(compute_shader_module)
		.setPName(compute_entry);

	auto pipeline_info = vk::ComputePipelineCreateInfo()
		.setStage(stage)
		.setLayout(layout);

	auto [result, pipeline] = device.logical.createComputePipeline(nullptr, pipeline_info, nullptr);
	assertion(result == vk::Result::eSuccess, "failed to compile pipeline");

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

	auto viewport = vk::Viewport()
		.setX(0.0f)
		.setY(0.0f)
		.setWidth(float(options.extent.width))
		.setHeight(float(options.extent.height))
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);

	auto scissor = vk::Rect2D()
		.setOffset({ 0, 0 })
		.setExtent(options.extent);

	auto viewport_state = vk::PipelineViewportStateCreateInfo()
		.setViewports(viewport)
		.setScissors(scissor);

	auto rasterization = vk::PipelineRasterizationStateCreateInfo()
		.setDepthClampEnable(false)
		.setRasterizerDiscardEnable(false)
		.setPolygonMode(options.polygon_mode)
		.setCullMode(options.cull_mode)
		.setFrontFace(vk::FrontFace::eCounterClockwise)
		.setDepthBiasEnable(false)
		.setLineWidth(1.0f);

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
		.setRenderPass(render_pass)
		.setSubpass(0);

	auto [result, pipeline] = device.logical.createGraphicsPipeline(nullptr, pipeline_info, nullptr);
	assertion(result == vk::Result::eSuccess, "failed to compile mesh shading pipeline");

	return pipeline;
}

} // namespace rcgp
