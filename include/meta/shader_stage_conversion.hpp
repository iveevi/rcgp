#pragma once

#include "../rhi/vk.hpp"
#include "../dsl/enumerations.hpp"

namespace rcgp {

consteval VkShaderStageFlagBits stage_to_flag(ShaderStage S)
{
	switch (S) {
	case ShaderStage::eVertex: return VK_SHADER_STAGE_VERTEX_BIT;
	case ShaderStage::eFragment: return VK_SHADER_STAGE_FRAGMENT_BIT;
	case ShaderStage::eCompute: return VK_SHADER_STAGE_COMPUTE_BIT;
	case ShaderStage::eTask: return VK_SHADER_STAGE_TASK_BIT_EXT;
	case ShaderStage::eMesh: return VK_SHADER_STAGE_MESH_BIT_EXT;
	default:
		return VK_SHADER_STAGE_ALL;
	}
}

template <ShaderStage ... Ss>
consteval VkShaderStageFlags stage_flags_of()
{
	return (stage_to_flag(Ss) | ... | VkShaderStageFlags(0));
}

} // namespace rcgp
