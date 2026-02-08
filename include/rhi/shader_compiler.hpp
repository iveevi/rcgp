#pragma once

#include <string>
#include <vector>

#include "../dsl/enumerations.hpp"

namespace rcgp {

struct ShaderCompiler {
	bool debug_info = true;
	uint32_t version = 460;

	std::vector <uint32_t> glsl_to_spirv(const std::string &glsl, const ShaderStage &stage) const;
};

} // namespace rcgp
