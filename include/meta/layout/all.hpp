#pragma once

#include "std430.hpp"

namespace layouts {

template <typename T>
using std430 = std430::layout_engine <T>;

} // namespace layouts
