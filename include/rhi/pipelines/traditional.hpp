#pragma once

#include <vulkan/vulkan.hpp>

#include "../device.hpp"

struct TraditionalGraphicsPipeline : vk::Pipeline {
	vk::PipelineLayout layout;

	struct Info {
		vk::ShaderModule vmodule;
		vk::ShaderModule fmodule;
		vk::RenderPass renderpass;
		vk::Extent2D extent;
		vk::ArrayProxy <const vk::VertexInputBindingDescription> bindings;
		vk::ArrayProxy <const vk::VertexInputAttributeDescription> attributes;
		vk::PipelineLayout layout = {};
	};

	TraditionalGraphicsPipeline() = default;
	TraditionalGraphicsPipeline(vk::Pipeline pipeline, vk::PipelineLayout layout_);

	static TraditionalGraphicsPipeline from(const Device &device, const vk::detail::DispatchLoaderDynamic &ldl, const Info &info);
};
