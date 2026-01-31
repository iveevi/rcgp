#pragma once

#include <cstddef>

#ifdef RCGP_SUPPORT_GLM
#include <glm/glm.hpp>
#endif

namespace rcgp::pod {

template <size_t D, typename T>
struct vec {
	T data[D];

	constexpr vec() = default;

#ifdef RCGP_SUPPORT_GLM
	vec(const glm::vec <D, T> &rhs) {
		for (size_t i = 0; i < D; ++i)
			data[i] = rhs[i];
	}

	operator glm::vec <D, T> () const {
		glm::vec <D, T> out(0);
		for (size_t i = 0; i < D; ++i)
			out[i] = data[i];
		return out;
	}
#endif
};

template <size_t C, size_t R, typename T>
struct mat {
	vec <R, T> data[C];

	constexpr mat() = default;

#ifdef RCGP_SUPPORT_GLM
	mat(const glm::mat <C, R, T> &rhs) {
		for (size_t c = 0; c < C; ++c) {
			for (size_t r = 0; r < R; ++r)
				data[c].data[r] = rhs[c][r];
		}
	}

	operator glm::mat <C, R, T> () const {
		glm::mat <C, R, T> out(0);
		for (size_t c = 0; c < C; ++c) {
			for (size_t r = 0; r < R; ++r)
				out[c][r] = data[c].data[r];
		}
		return out;
	}
#endif
};

} // namespace rcgp::pod
