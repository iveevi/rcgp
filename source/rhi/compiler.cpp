#include "rhi/compiler.hpp"

#include <fmt/printf.h>

#include <glslang/Public/ResourceLimits.h>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>

Compiler Compiler::from(const Device &device, const Info &info)
{
	(void) info;
	return Compiler{device.logical};
}

std::vector <uint32_t> Compiler::glsl_to_spirv(const std::string &glsl, const EShLanguage &stage) const
{
	const char *cstr[] = { glsl.c_str() };

	glslang::SpvOptions options;
	options.generateDebugInfo = true;

	glslang::TShader shader(stage);

	shader.setStrings(cstr, 1);
	shader.setEnvTarget(
		glslang::EShTargetLanguage::EShTargetSpv,
		glslang::EShTargetLanguageVersion::EShTargetSpv_1_6
	);

	EShMessages messages = EShMessages {
		EShMsgDefault
		| EShMsgSpvRules
		| EShMsgVulkanRules
		| EShMsgDebugInfo
	};

	if (!shader.parse(GetDefaultResources(), 460, false, messages)) {
		std::string log = shader.getInfoLog();
		fmt::println("failed to compile to SPIRV:\n{}", log);
		return {};
	}

	glslang::TProgram program;
	program.addShader(&shader);

	if (!program.link(messages)) {
		std::string log = program.getInfoLog();
		fmt::println("failed to link SPIRV code:\n{}", log);
		return {};
	}

	std::vector <uint32_t> spirv;
	glslang::GlslangToSpv(*program.getIntermediate(stage), spirv, &options);
	return spirv;
}
