#pragma once

#include <type_traits>

#include "../dsl/tracer.hpp"
#include "../util/sequence.hpp"

enum class Stage {
	Undefined,

	// Standard stages before compilation
	RepresentationalVertex,
	RepresentationalFragment,
	RepresentationalCompute,

	// Standard stages after compilation
	Vertex,
	Fragment,
	Compute,
};

constexpr bool is_representational(Stage S)
{
	switch (S) {
	case Stage::RepresentationalVertex:
	case Stage::RepresentationalFragment:
	case Stage::RepresentationalCompute:
		return true;
	default:
		break;
	}

	return false;
}

// constexpr Stage compiled_shader_stage(Stage S)
// {
// 	switch (S) {
// 	case Stage::RepresentationalVertex:
// 		return Stage::Vertex;
// 	case Stage::RepresentationalFragment:
// 		return Stage::Fragment;
// 	case Stage::RepresentationalCompute:
// 		return Stage::Compute;
// 	default:
// 		break;
// 	}
//
// 	return Stage::Undefined;
// }

template <Stage S, typename R, typename ... Args>
struct stage {};

template <Stage S, typename R, typename ... Args>
requires (is_representational(S))
struct stage <S, R, Args...> : Block {};
