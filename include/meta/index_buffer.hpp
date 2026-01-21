#pragma once

#include "layout/all.hpp"
#include "mirror_buffer.hpp"

enum class Topology {
	eTriangleList,
	eTriangleFan,
};

// Index buffer type helpers
template <Topology T, typename I>
using topology_element_t = std::conditional_t <
	T == Topology::eTriangleList,
	vector <I, 3>,
	scalar <I>
>;

template <Topology T, typename I>
using IndexBuffer = IndexMirrorBuffer <
	array <topology_element_t <T, I>>,
	layouts::scalar
>;
