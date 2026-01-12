#pragma once

#include "device.hpp"

struct RasterizationOptions {
	// TODO: dynamic extent or fixed extent
	const vk::Extent2D &extent;
	bool depth_test;
};

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
);
