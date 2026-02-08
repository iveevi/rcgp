#pragma once

#include "device.hpp"
#include "../util/variant.hpp"

namespace rcgp {

struct RasterizationOptions {
	VkExtent2D extent {};
	bool depth_test = false;
	VkCullModeFlags cull_mode = 0;
	VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
	bool alpha_blend = false;
};

using RenderState = variant <VkRenderPass, VkPipelineRenderingCreateInfo>;

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
);

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
);

VkPipeline compile_compute_pipeline(
	const Device &device,
	VkShaderModule compute_shader_module,
	const char *compute_entry,
	VkPipelineLayout layout
);

} // namespace rcgp
