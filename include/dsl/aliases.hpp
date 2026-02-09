#pragma once

#include "scalar.hpp"
#include "vector.hpp"
#include "matrix.hpp"

namespace rcgp {

using boolean = scalar <bool>;
using i32 = scalar <int32_t>;
using u32 = scalar <uint32_t>;
using f32 = scalar <float>;

// GLSL style aliases
using vec2 = vector <float, 2>;
using vec3 = vector <float, 3>;
using vec4 = vector <float, 4>;

using ivec2 = vector <int32_t, 2>;
using ivec3 = vector <int32_t, 3>;
using ivec4 = vector <int32_t, 4>;

using uvec2 = vector <uint32_t, 2>;
using uvec3 = vector <uint32_t, 3>;
using uvec4 = vector <uint32_t, 4>;

using mat3 = matrix <float, 3, 3>;
using mat4 = matrix <float, 4, 4>;

// HLSL style aliases
using float2 = vector <float, 2>;
using float3 = vector <float, 3>;
using float4 = vector <float, 4>;

using int2 = vector <int32_t, 2>;
using int3 = vector <int32_t, 3>;
using int4 = vector <int32_t, 4>;

using uint2 = vector <uint32_t, 2>;
using uint3 = vector <uint32_t, 3>;
using uint4 = vector <uint32_t, 4>;

using float3x3 = matrix <float, 3, 3>;
using float4x4 = matrix <float, 4, 4>;

} // namespace rcgp
