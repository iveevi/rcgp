#pragma once

#include <utility>

#include "instruction_nodes.hpp"

namespace rcgp {

enum class OptimizationPhases : uint8_t {
	eNone = 0b0,
	eReadability = 0b1,
	eDeadCodeElimination = 0b10,
	eLocalElision = 0b100,
	eReuse = 0b1000,
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
