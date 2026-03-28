#include "rhi/sbt.hpp"
#include "rhi/device.hpp"

#include <cstring>
#include <vector>

namespace rcgp {

auto Device::build_sbt(
	vk::Pipeline pipeline,
	uint32_t miss_count,
	uint32_t hit_count
) const -> ShaderBindingTable
{
	vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rt_props;
	vk::PhysicalDeviceProperties2 phys_props2;
	phys_props2.pNext = &rt_props;
	physical.getProperties2(&phys_props2);

	const uint32_t handle_size = rt_props.shaderGroupHandleSize;
	const uint32_t handle_alignment = rt_props.shaderGroupHandleAlignment;
	const uint32_t base_alignment = rt_props.shaderGroupBaseAlignment;

	const uint32_t entry_stride = (handle_size + handle_alignment - 1) & ~(handle_alignment - 1);

	auto region_size = [&](uint32_t count) -> uint32_t {
		uint32_t sz = count * entry_stride;
		return (sz + base_alignment - 1) & ~(base_alignment - 1);
	};

	uint32_t raygen_region = region_size(1);
	uint32_t miss_region = region_size(miss_count);
	uint32_t hit_region = region_size(hit_count);
	uint32_t sbt_total = raygen_region + miss_region + hit_region;

	uint32_t group_count = 1 + miss_count + hit_count;
	std::vector <uint8_t> handles(group_count * handle_size);
	auto result = logical.getRayTracingShaderGroupHandlesKHR(
		pipeline, 0, group_count, handles.size(), handles.data(), loader
	);
	(void)result;

	auto sbt_buf = Buffer::from(*this,
		sbt_total,
		vk::BufferUsageFlagBits::eShaderBindingTableKHR
		| vk::BufferUsageFlagBits::eShaderDeviceAddress,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	);

	auto *dst = static_cast <uint8_t *> (logical.mapMemory(sbt_buf.backing, 0, sbt_total));

	// Raygen group (always 1)
	std::memcpy(dst, handles.data(), handle_size);

	// Miss groups
	for (uint32_t i = 0; i < miss_count; i++) {
		std::memcpy(
			dst + raygen_region + i * entry_stride,
			handles.data() + (1 + i) * handle_size,
			handle_size
		);
	}

	// Hit groups
	for (uint32_t i = 0; i < hit_count; i++) {
		std::memcpy(
			dst + raygen_region + miss_region + i * entry_stride,
			handles.data() + (1 + miss_count + i) * handle_size,
			handle_size
		);
	}

	logical.unmapMemory(sbt_buf.backing);

	auto sbt_addr = get_address(sbt_buf);

	return ShaderBindingTable {
		.buffer = sbt_buf,
		.raygen = vk::StridedDeviceAddressRegionKHR()
			.setDeviceAddress(sbt_addr)
			.setStride(raygen_region)
			.setSize(raygen_region),
		.miss = vk::StridedDeviceAddressRegionKHR()
			.setDeviceAddress(sbt_addr + raygen_region)
			.setStride(entry_stride)
			.setSize(miss_region),
		.hit = vk::StridedDeviceAddressRegionKHR()
			.setDeviceAddress(sbt_addr + raygen_region + miss_region)
			.setStride(entry_stride)
			.setSize(hit_region),
		.callable = vk::StridedDeviceAddressRegionKHR(),
	};
}

} // namespace rcgp
