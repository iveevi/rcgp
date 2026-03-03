#pragma once

#include "resources.hpp"

namespace rcgp {

// Storage buffers
template <typename T, template <typename> typename L = layouts::std430>
using RStorageBuffer = StorageBuffer <T, L, GlobalResourceAccess::eRead>;

template <typename T, template <typename> typename L = layouts::std430>
using WStorageBuffer = StorageBuffer <T, L, GlobalResourceAccess::eWrite>;

template <typename T, template <typename> typename L = layouts::std430>
using RWStorageBuffer = StorageBuffer <T, L, GlobalResourceAccess::eReadWrite>;

// Storage buffers of arrays
template <typename T, template <typename> typename L = layouts::std430, GlobalResourceAccess A = GlobalResourceAccess::eReadWrite>
using ArrayBuffer = StorageBuffer <array <T>, L, A>;

template <typename T, template <typename> typename L = layouts::std430>
using RArrayBuffer = ArrayBuffer <T, L, GlobalResourceAccess::eRead>;

template <typename T, template <typename> typename L = layouts::std430>
using WArrayBuffer = ArrayBuffer <T, L, GlobalResourceAccess::eWrite>;

template <typename T, template <typename> typename L = layouts::std430>
using RWArrayBuffer = ArrayBuffer <T, L, GlobalResourceAccess::eReadWrite>;

// Samplers
using Sampler1D = Sampler <float, 1>;
using Sampler2D = Sampler <float, 2>;
using Sampler3D = Sampler <float, 3>;

} // namespace rcgp
