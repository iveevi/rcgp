#pragma once

#include <cstring>
#include <vector>

#ifdef RCGP_SUPPORT_GLM
#include <glm/glm.hpp>
#endif

#include "buffer.hpp"

namespace rcgp {

struct Device;
struct CommandBuffer;

struct AccelerationStructure {
	vk::AccelerationStructureKHR handle;
	Buffer buffer;

	void destroy(const Device &device);

	static auto from(
		const Device &device,
		const vk::AccelerationStructureBuildGeometryInfoKHR &build_info,
		uint32_t max_primitive_count
	) -> std::tuple <AccelerationStructure, uint32_t>;
};

// Row-major 3x4 affine transform (matches VkTransformMatrixKHR layout)
struct XlasTransform {
	float data[3][4] = {
		{ 1, 0, 0, 0 },
		{ 0, 1, 0, 0 },
		{ 0, 0, 1, 0 },
	};

#ifdef RCGP_SUPPORT_GLM
	XlasTransform() = default;
	XlasTransform(const glm::mat4 &m) {
		// glm is column-major; VkTransformMatrixKHR is row-major 3x4
		for (int r = 0; r < 3; r++)
			for (int c = 0; c < 4; c++)
				data[r][c] = m[c][r];
	}
#endif
};

struct XlasInstance {
	uint32_t index;
	XlasTransform transform;
	AccelerationStructure blas;
};

} // namespace rcgp
