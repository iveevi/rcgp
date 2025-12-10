#pragma once

#include <type_traits>

#include "../dsl/tracer.hpp"
#include "../util/sequence.hpp"

enum class ShaderStage {
	Undefined,

	// Standard stages after compilation
	Vertex,
	Fragment,
	Compute,
};

constexpr vk::ShaderStageFlags stage_to_flag(ShaderStage S)
{
	switch (S) {
	case ShaderStage::Vertex: return vk::ShaderStageFlagBits::eVertex;
	case ShaderStage::Fragment: return vk::ShaderStageFlagBits::eFragment;
	default:
		return vk::ShaderStageFlagBits::eAll;
	}
}

template <ShaderStage S, typename R, typename ... Args>
struct shader_stage : Block {};
