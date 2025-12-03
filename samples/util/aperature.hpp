#pragma once

#include <glm/glm.hpp>

struct Aperature {
	float aspect = 45.0f;
	float fovy = 1.0f;
	float near = 0.1f;
	float far = 10000.0f;

	glm::mat4 projection_matrix() const {
		auto rad = glm::radians(fovy);
		auto tan_half_fovy = std::tan(rad / 2.0f);

		glm::mat4 ret(1.0f);
		ret[0][0] = 1.0f / (aspect * tan_half_fovy);
		ret[1][1] = 1.0f / (tan_half_fovy);
		ret[2][2] = -(far + near) / (far - near);
		ret[2][3] = -1.0f;
		ret[3][2] = -2.0f * (far * near) / (far - near);
		return ret;
	}
};
