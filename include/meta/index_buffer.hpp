#pragma once

#include "scalar.hpp"
#include "mirror_buffer.hpp"

enum class Topology {
	eTriangleList,
	eTriangleFan,
};

consteval vk::PrimitiveTopology translate_topology(Topology T)
{
	using enum vk::PrimitiveTopology;

	switch (T) {
	case Topology::eTriangleList: return eTriangleList;
	case Topology::eTriangleFan: return eTriangleFan;
	}

	return vk::PrimitiveTopology::eTriangleList;
}

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
