#pragma once

#include <vulkan/vulkan.hpp>

#include "../device.hpp"

struct RasterizationPipeline : vk::Pipeline {
	vk::PipelineLayout layout;

	struct Info {
		vk::ShaderModule vmodule;
		vk::ShaderModule fmodule;
		vk::RenderPass renderpass;
		vk::Extent2D extent;
		vk::ArrayProxy <const vk::VertexInputBindingDescription> bindings;
		vk::ArrayProxy <const vk::VertexInputAttributeDescription> attributes;
		vk::PipelineLayout layout = {};
		bool depth_test = true;
	};

	RasterizationPipeline() = default;
	RasterizationPipeline(vk::Pipeline pipeline, vk::PipelineLayout layout_);

	static RasterizationPipeline from(const Device &device, const vk::detail::DispatchLoaderDynamic &ldl, const Info &info);
};
