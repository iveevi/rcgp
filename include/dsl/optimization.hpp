#pragma once

#include <utility>

#include "instruction_nodes.hpp"

namespace rcgp {

// TODO: pass for readability -- for GLSL generator
enum class OptimizationPhases : uint8_t {
        eDeadCodeElimination = 0b1,
	eLocalElision = 0b10,
};

inline auto operator|(OptimizationPhases a, OptimizationPhases b)
{
	return OptimizationPhases(std::to_underlying(a) | std::to_underlying(b));
}

inline bool has_flag(OptimizationPhases a, OptimizationPhases b)
{
	return (std::to_underlying(a) & std::to_underlying(b));
}

void optimize(const SharedBlockReference &sbr, OptimizationPhases phases);

} // namespace rcgp
