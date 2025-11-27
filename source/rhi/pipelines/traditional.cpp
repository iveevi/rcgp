#include <array>

#include "rhi/pipelines/traditional.hpp"

TraditionalGraphicsPipeline::TraditionalGraphicsPipeline(vk::Pipeline pipeline, vk::PipelineLayout layout_)
	: vk::Pipeline(pipeline), layout(layout_)
{}

TraditionalGraphicsPipeline TraditionalGraphicsPipeline::from(const Device &device, const vk::detail::DispatchLoaderDynamic &ldl, const Info &info)
{
	auto shader_stages = std::array {
		vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setModule(info.vmodule)
			.setPName("main"),
		vk::PipelineShaderStageCreateInfo()
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setModule(info.fmodule)
			.setPName("main"),
	};

	auto vertex_input = vk::PipelineVertexInputStateCreateInfo()
		.setVertexBindingDescriptions(info.bindings)
		.setVertexAttributeDescriptions(info.attributes);

	auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo()
		.setTopology(vk::PrimitiveTopology::eTriangleList)
		.setPrimitiveRestartEnable(false);

	auto viewport = vk::Viewport()
		.setX(0.0f)
		.setY(0.0f)
		.setWidth(static_cast <float> (info.extent.width))
		.setHeight(static_cast <float> (info.extent.height))
		.setMinDepth(0.0f)
		.setMaxDepth(1.0f);

	auto scissor = vk::Rect2D()
		.setOffset({ 0, 0 })
		.setExtent(info.extent);

	auto viewport_state = vk::PipelineViewportStateCreateInfo()
		.setViewports(viewport)
		.setScissors(scissor);

	auto rasterization = vk::PipelineRasterizationStateCreateInfo()
		.setDepthClampEnable(false)
		.setRasterizerDiscardEnable(false)
		.setPolygonMode(vk::PolygonMode::eFill)
		.setCullMode(vk::CullModeFlagBits::eBack)
		.setFrontFace(vk::FrontFace::eClockwise)
		.setDepthBiasEnable(false)
		.setLineWidth(1.0f);

	auto multisampling = vk::PipelineMultisampleStateCreateInfo()
		.setRasterizationSamples(vk::SampleCountFlagBits::e1)
		.setSampleShadingEnable(false)
		.setMinSampleShading(1.0f);

	auto color_blend_attachment = vk::PipelineColorBlendAttachmentState()
		.setBlendEnable(false)
		.setColorWriteMask(
			vk::ColorComponentFlagBits::eR |
			vk::ColorComponentFlagBits::eG |
			vk::ColorComponentFlagBits::eB |
			vk::ColorComponentFlagBits::eA
		);

	auto color_blend = vk::PipelineColorBlendStateCreateInfo()
		.setAttachments(color_blend_attachment);

	auto pipeline_layout = info.layout;
	if (!pipeline_layout)
		pipeline_layout = device.logical.createPipelineLayout(vk::PipelineLayoutCreateInfo(), nullptr, ldl);

	auto pipeline_info = vk::GraphicsPipelineCreateInfo()
		.setLayout(pipeline_layout)
		.setPInputAssemblyState(&input_assembly)
		.setPVertexInputState(&vertex_input)
		.setPRasterizationState(&rasterization)
		.setPMultisampleState(&multisampling)
		.setPColorBlendState(&color_blend)
		.setPViewportState(&viewport_state)
		.setStages(shader_stages)
		.setRenderPass(info.renderpass)
		.setSubpass(0);

	auto [result, pipeline] = device.logical.createGraphicsPipeline(nullptr, pipeline_info, nullptr, ldl);
	// (void)result; // TODO: handle failures gracefully

	return TraditionalGraphicsPipeline(pipeline, pipeline_layout);
}
