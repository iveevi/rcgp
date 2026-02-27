#pragma once

#include <optional>

#include "device.hpp"
#include "../util/variant.hpp"

namespace rcgp {

struct RasterizationOptions {
	// If omitted, viewport/scissor are expected to be set dynamically.
	std::optional <vk::Extent2D> extent;
	bool depth_test;
	vk::CullModeFlags cull_mode;
	vk::PolygonMode polygon_mode;
	float line_width = 1.0f;
	bool alpha_blend = false;
};

using RenderState = variant <vk::RenderPass, vk::PipelineRenderingCreateInfo>;

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
);

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
);

vk::Pipeline compile_compute_pipeline(
	const Device &device,
	const vk::ShaderModule &compute_shader_module,
	const char *compute_entry,
	const vk::PipelineLayout &layout
);

} // namespace rcgp
