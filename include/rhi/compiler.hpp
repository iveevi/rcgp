#pragma once

#include <string>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <glslang/Public/ShaderLang.h>

#include "device.hpp"

struct Compiler {
	const vk::Device &reference;

	struct Info {
		// TODO: pipeline state?
	};

	static Compiler from(const Device &device, const Info &info);

	std::vector <uint32_t> glsl_to_spirv(const std::string &glsl, const EShLanguage &stage) const;
};
