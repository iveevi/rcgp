#pragma once

#include "reflection.hpp"

// TODO: needs a layout...
template <typename T>
struct ParameterBlock : public T {
	// TODO: lock the members until its $use-ed
	using reflection = parameter_block_reflection <
		typename reflection_expander <T> ::type
	>;
};

template <typename T>
struct RayPayload : T {};
