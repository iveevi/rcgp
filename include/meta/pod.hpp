#pragma once

#ifdef RCGP_SUPPORT_GLM

#include <glm/glm.hpp>

namespace rcgp::pod {

template <size_t D, typename T>
using vec = glm::vec <D, T>;

template <size_t C, size_t R, typename T>
using mat = glm::mat <C, R, T>;

} // namespace rcgp::pod

#else

#include <cstddef>

namespace rcgp::pod {

template <size_t D, typename T>
using vec = T[D];

template <size_t C, size_t R, typename T>
using mat = vec <R, T> [C];

} // namespace rcgp::pod

#endif
