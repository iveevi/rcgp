#pragma once

#include <type_traits>

#include "../dsl/tracer.hpp"
#include "../util/sequence.hpp"

enum class Stage {
	Undefined,

	// Standard stages after compilation
	Vertex,
	Fragment,
	Compute,
};

template <Stage S, typename R, typename ... Args>
struct stage : Block {};
