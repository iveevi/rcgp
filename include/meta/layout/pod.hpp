#pragma once

#include <cstddef>

#ifdef GLM_VERSION_MAJOR
#include <glm/glm.hpp>
#endif

namespace pod {

template <size_t D, typename T>
struct vec {
	T data[D];

#ifdef GLM_VERSION_MAJOR
	template <glm::qualifier Q>
	explicit vec(const glm::vec <D, T, Q> &rhs) {
		for (size_t i = 0; i < D; ++i)
			data[i] = rhs[i];
	}

	template <glm::qualifier Q>
	operator glm::vec <D, T, Q>() const {
		glm::vec <D, T, Q> out(0);
		for (size_t i = 0; i < D; ++i)
			out[i] = data[i];
		return out;
	}
#endif
};

template <size_t C, size_t R, typename T>
struct mat {
	vec <R, T> data[C];

#ifdef GLM_VERSION_MAJOR
	template <glm::qualifier Q>
	explicit mat(const glm::mat <C, R, T, Q> &rhs) {
		for (size_t c = 0; c < C; ++c) {
			for (size_t r = 0; r < R; ++r)
				data[c].data[r] = rhs[c][r];
		}
	}

	template <glm::qualifier Q>
	operator glm::mat <C, R, T, Q>() const {
		glm::mat <C, R, T, Q> out(0);
		for (size_t c = 0; c < C; ++c) {
			for (size_t r = 0; r < R; ++r)
				out[c][r] = data[c].data[r];
		}
		return out;
	}
#endif
};

} // namespace pod
