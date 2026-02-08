#pragma once

#include "layouts.hpp"
#include "mirror_buffer.hpp"

namespace rcgp {

enum class Topology {
	eTriangleList,
	eTriangleFan,
};

consteval VkPrimitiveTopology translate_topology(Topology T)
{
	switch (T) {
	case Topology::eTriangleList: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	case Topology::eTriangleFan: return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
	}

	return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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

} // namespace rcgp
