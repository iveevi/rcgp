#pragma once

#include <type_traits>

#include "meta.hpp"
#include "../dsl/tracer.hpp"

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

// TODO: static string conversion...

template <Stage S, typename R, typename ... Args>
struct decomposition {
	using args = sequence <Args...>;
};

template <typename R, typename ... Args>
struct decomposition <Stage::Vertex, R, Args...> {
	// using 
	
	static constexpr bool is_svpos_defined = false;
	static constexpr bool is_topology_consistent = false;
};

// TODO: stage_base for common reflection stuff...
// then specialize stage implementations to store differently
// e.g. RepresentationalX is a TracerRecord
// and X contains a Vulkan shader module and Vulkan device
template <Stage S, typename R, typename ... Args>
struct stage {
	// TODO: reflection...
	// using reflection = decltype(function_reflection_generator <R, Args...> ());
};

template <Stage S, typename R, typename ... Args>
requires (is_representational(S))
struct stage <S, R, Args...> : Block {
};

// template <typename T>
// struct is_stage : std::false_type {};
//
// template <Stage S, typename R, typename ... Args>
// struct is_stage <stage <S, R, Args...>> : std::true_type {};
