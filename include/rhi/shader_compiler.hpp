#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <glslang/Public/ShaderLang.h>

#include "device.hpp"

struct ShaderCompiler {
	const vk::Device &device;

	std::vector <uint32_t> glsl_to_spirv(const std::string &glsl, const EShLanguage &stage) const;
	
	vk::ShaderModule spirv_to_shader_module(const std::vector <uint32_t> &spirv) const;

	struct Options {
		bool debug_info = true;
		// TODO: pipeline state/cache?
	};

	static ShaderCompiler from(const Device &device, const Options &info);
};
