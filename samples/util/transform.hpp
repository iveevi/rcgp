#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

struct Transform {
	glm::vec3 translation = glm::vec3(0.0f);
	glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	glm::vec3 scaling = glm::vec3(1.0f);

	glm::vec3 forward() const {
		static constexpr auto D = glm::vec4(0, 0, 1, 0);
		return glm::normalize(glm::vec3(rotation * D));
	}

	glm::vec3 right() const {
		static constexpr auto D = glm::vec4(1, 0, 0, 0);
		return glm::normalize(glm::vec3(rotation * D));
	}

	glm::vec3 up() const {
		static constexpr auto D = glm::vec4(0, 1, 0, 0);
		return glm::normalize(glm::vec3(rotation * D));
	}

	glm::mat4 matrix() const {
		auto t = glm::translate(glm::mat4(1.0f), translation);
		auto r = glm::toMat4(rotation);
		auto s = glm::scale(glm::mat4(1.0f), scaling);
		return t * r * s;
	}

	glm::mat4 view_matrix() const {
		return glm::lookAt(translation, translation + forward(), up());
	}
};
