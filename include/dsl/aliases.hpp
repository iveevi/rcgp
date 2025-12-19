#pragma once

#include "vector.hpp"
#include "matrix.hpp"

// TODO: move to aliases.hpp
using vec2 = vector <float, 2>;
using vec3 = vector <float, 3>;
using vec4 = vector <float, 4>;

using ivec3 = vector <int32_t, 3>;

using float2 = vector <float, 2>;
using float3 = vector <float, 3>;
using float4 = vector <float, 4>;

using int2 = vector <int32_t, 2>;
using int3 = vector <int32_t, 3>;
using int4 = vector <int32_t, 4>;

using mat3 = matrix <float, 3, 3>;
using mat4 = matrix <float, 4, 4>;
