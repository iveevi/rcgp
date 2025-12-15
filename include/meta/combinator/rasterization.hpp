#pragma once

#include "../../dsl/generators/glsl.hpp"
#include "../../rhi/compiler.hpp"
#include "../collect_gvrs.hpp"
#include "../collect_streams.hpp"
#include "../implicit_context.hpp"
#include "../group_allocation.hpp"
#include "../pipeline/rasterization.hpp"
#include "../shader_stage.hpp"

// TODO: group allocation record should be a proper type...
// template <...> struct group_allocation { get_set_for <ref> = ... };
template <auto &ref, auto &... refs, size_t ... Is>
constexpr auto group_allocation_set_for(const std::tuple <group_allocation_record <refs, Is>...> &)
{
	constexpr auto matches = std::array {
		std::same_as <
			reference <ref>,
			reference <refs>
		>...
	};

	constexpr auto index = first_on(matches);
	if constexpr (index < 0) {
		static_assert(false, "reference not in group allocation");
		return 0;
	} else {
		return Is...[index];
	}
}

template <auto &... refs, size_t ... Is>
auto sequence_to_group_allocation_impl(const sequence <reference <refs>...> &, const std::index_sequence <Is...> &)
{
	return std::make_tuple(group_allocation_record <refs, Is> ()...);
}

template <typename ... Ts>
auto sequence_to_group_allocation(const sequence <Ts...> &)
{
	return sequence_to_group_allocation_impl(
		sequence <typename Ts::type...> ::singleton,
		std::make_index_sequence <sizeof...(Ts)> ()
	);
}

// TODO: flesh out with an impl method with handles things recursively...
// TODO: fallback...
template <auto &ref, ShaderStage ... Ss>
auto reference_to_dsl_and_pcr(const Device &device, const stage_wrapper <reference <ref>, Ss...> &)
{
	// TODO: refactor to reference_to_descriptor_set_layout
	using base = typename reference <ref> ::base;
	using R = expand_reflection_t <base>;

	// TODO: collect all dslbs and pc ranges recursively
	// TODO: if its a resource group then we need to filter it...
	auto dslbs = std::array {
		vk::DescriptorSetLayoutBinding()
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setStageFlags((stage_to_flag(Ss) | ...))
	};

	auto dsl_info = vk::DescriptorSetLayoutCreateInfo()
		.setBindings(dslbs);

	// TODO: return the dsl and pc ranges separately
	return device.logical.createDescriptorSetLayout(dsl_info);
}

template <typename ... Ts>
auto reference_sequence_to_dsl_and_pcr(const Device &device, const sequence <Ts...> &)
{
	// TODO: separate arrays for dsl and pc ranges
	if constexpr (sizeof...(Ts) == 0) {
		return std::array <vk::DescriptorSetLayout, 0> ();
	} else {
		return std::array {
			reference_to_dsl_and_pcr(device, Ts())...
		};
	}
}

template <typename ... Subpasses>
struct RasterizationPipelineCombinator {
	const Device &device;
	const Compiler &compiler;
	const RenderPass <Subpasses...> &render_pass;
	// TODO: dynamic extent or fixed extent
	const vk::Extent2D &extent;
	bool depth_test;

	template <typename VRet, typename ... As, typename FRet, typename ... Bs>
	auto operator()(shader_stage <ShaderStage::Vertex, VRet, As...> &vertex,
		 	shader_stage <ShaderStage::Fragment, FRet, Bs...> &fragment) const {
		// TODO: check between vshader output and fshader input
		// TODO: store # of attachments required from # of fshader outputs
		using vertex_icontext = find_implicit_context <As...> ::type;
		using fragment_icontext = find_implicit_context <Bs...> ::type;

		// Collect vertex attribute streams
		auto streams0 = sequence <> ::singleton;
		auto streams = add_stream_from_implicit_context(streams0, vertex_icontext());

		// Generate vertex input bindings and attributes
		auto vbindings = sequence_to_vertex_bindings(streams);
		auto vattributes = sequence_to_vertex_attributes(streams);

		// Collect global resources
		auto gvrs0 = sequence <> ::singleton;
		auto gvrs1 = add_gvr_from_implicit_context <ShaderStage::Vertex> (gvrs0, vertex_icontext());
		auto gvrs = add_gvr_from_implicit_context <ShaderStage::Fragment> (gvrs1, fragment_icontext());

		auto alloc = sequence_to_group_allocation(gvrs);
		auto gamap = new_allocation(alloc);
		vertex.apply_group_allocation_map(gamap);
		fragment.apply_group_allocation_map(gamap);

		// Compile the shaders
		auto vshader = generators::GLSL(vertex).generate();
		fmt::println("vertex shader:\n{}", vshader);

		auto fshader = generators::GLSL(fragment).generate();
		fmt::println("fragment shader:\n{}", fshader);
	
		auto vspv = compiler.glsl_to_spirv(vshader, EShLangVertex);
		auto fspv = compiler.glsl_to_spirv(fshader, EShLangFragment);

		auto vmodule = compiler.spirv_to_shader_module(vspv);
		auto fmodule = compiler.spirv_to_shader_module(fspv);

		// Generate the pipeline and descriptor set layouts
		auto dsls = reference_sequence_to_dsl_and_pcr(device, gvrs);
		auto layout_info = vk::PipelineLayoutCreateInfo().setSetLayouts(dsls);
		auto layout = device.logical.createPipelineLayout(layout_info);

		// Building the pipeline
		auto shader_stages = std::array {
			vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eVertex)
				.setModule(vmodule)
				.setPName("main"),
			vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eFragment)
				.setModule(fmodule)
				.setPName("main"),
		};

		// TODO: new_pipeline_vertex_input_state
		auto vertex_input = vk::PipelineVertexInputStateCreateInfo()
			.setVertexBindingDescriptions(vbindings)
			.setVertexAttributeDescriptions(vattributes);

		auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology(vk::PrimitiveTopology::eTriangleList)
			.setPrimitiveRestartEnable(false);

		auto viewport = vk::Viewport()
			.setX(0.0f)
			.setY(0.0f)
			.setWidth(float(extent.width))
			.setHeight(float(extent.height))
			.setMinDepth(0.0f)
			.setMaxDepth(1.0f);

		auto scissor = vk::Rect2D()
			.setOffset({ 0, 0 })
			.setExtent(extent);

		auto viewport_state = vk::PipelineViewportStateCreateInfo()
			.setViewports(viewport)
			.setScissors(scissor);

		auto rasterization = vk::PipelineRasterizationStateCreateInfo()
			.setDepthClampEnable(false)
			.setRasterizerDiscardEnable(false)
			.setPolygonMode(vk::PolygonMode::eFill)
			.setCullMode(vk::CullModeFlagBits::eBack)
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
			.setDepthTestEnable(depth_test)
			.setDepthWriteEnable(depth_test)
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

		return AnnotatedRasterizationPipeline <
			decltype(streams),
			decltype(alloc),
			dsls.size()
		> {
			.device = device.logical,
			.handle = pipeline,
			.layout = layout,
			.dsls = dsls,
		};
	}
};

// CTAD for aggregate initialization, deducing subpasses from the render pass.
template <typename ... Subpasses>
RasterizationPipelineCombinator(
	const Device &,
	const Compiler &,
	const RenderPass <Subpasses...> &,
	const vk::Extent2D &,
	bool
) -> RasterizationPipelineCombinator <Subpasses...>;
