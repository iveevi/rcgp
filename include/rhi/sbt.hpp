#pragma once

#include "buffer.hpp"

namespace rcgp {

struct ShaderBindingTable {
	Buffer buffer;
	vk::StridedDeviceAddressRegionKHR raygen;
	vk::StridedDeviceAddressRegionKHR miss;
	vk::StridedDeviceAddressRegionKHR hit;
	vk::StridedDeviceAddressRegionKHR callable;
};

} // namespace rcgp
