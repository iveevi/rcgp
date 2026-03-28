#include <span>

#include "rhi/xlas.hpp"
#include "rhi/device.hpp"
#include "rhi/command_buffer.hpp"

namespace rcgp {

auto AccelerationStructure::from(
	const Device &device,
	const vk::AccelerationStructureBuildGeometryInfoKHR &build_info,
	uint32_t max_primitive_count
) -> std::tuple <AccelerationStructure, uint32_t>
{
	auto sizes = device.logical.getAccelerationStructureBuildSizesKHR(
		vk::AccelerationStructureBuildTypeKHR::eDevice,
		build_info, max_primitive_count, device.loader
	);

	AccelerationStructure result;
	result.buffer = Buffer::from(device,
		sizes.accelerationStructureSize,
		vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR
		       | vk::BufferUsageFlagBits::eShaderDeviceAddress,
		vk::MemoryPropertyFlagBits::eDeviceLocal
	);
	result.handle = device.logical.createAccelerationStructureKHR(
		vk::AccelerationStructureCreateInfoKHR()
			.setBuffer(result.buffer.handle)
			.setSize(sizes.accelerationStructureSize)
			.setType(build_info.type),
		nullptr, device.loader
	);

	return std::tuple { result, sizes.buildScratchSize };
}

auto Device::build_blas(
	const CommandBuffer &cmd,
	const Buffer &vertices,
	const Buffer &indices,
	size_t vertex_count,
	size_t primitive_count
) const -> std::tuple <AccelerationStructure, Buffer>
{
	auto geometry_data = vk::AccelerationStructureGeometryDataKHR()
		.setTriangles(vk::AccelerationStructureGeometryTrianglesDataKHR()
			.setVertexFormat(vk::Format::eR32G32B32Sfloat)
			.setVertexData(get_address(vertices))
			.setVertexStride(sizeof(float) * 3)
			.setMaxVertex(vertex_count - 1)
			.setIndexType(vk::IndexType::eUint32)
			.setIndexData(get_address(indices)));

	auto geometry = vk::AccelerationStructureGeometryKHR()
		.setGeometryType(vk::GeometryTypeKHR::eTriangles)
		.setFlags(vk::GeometryFlagBitsKHR::eOpaque)
		.setGeometry(geometry_data);

	auto build_info = vk::AccelerationStructureBuildGeometryInfoKHR()
		.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
		.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
		.setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
		.setGeometries(geometry);

	auto [blas, scratch_size] = AccelerationStructure::from(*this, build_info, primitive_count);

	auto scratch = Buffer::from(*this,
		scratch_size,
		vk::BufferUsageFlagBits::eStorageBuffer
		| vk::BufferUsageFlagBits::eShaderDeviceAddress,
		vk::MemoryPropertyFlagBits::eDeviceLocal
	);

	build_info
		.setDstAccelerationStructure(blas.handle)
		.setScratchData(get_address(scratch));

	auto range = vk::AccelerationStructureBuildRangeInfoKHR()
		.setPrimitiveCount(primitive_count);

	cmd.buildAccelerationStructuresKHR(build_info, &range, loader);

	return std::tuple { blas, scratch };
}

auto Device::build_tlas(
	const CommandBuffer &cmd,
	const std::vector <XlasInstance> &list
) const -> std::tuple <AccelerationStructure, Buffer>
{
	std::vector <vk::AccelerationStructureInstanceKHR> instances;

	instances.reserve(list.size());
	for (auto &x : list) {
		vk::TransformMatrixKHR xform;
		static_assert(sizeof(xform) == sizeof(x.transform.data));
		std::memcpy(&xform, x.transform.data, sizeof(xform));

		auto addr = get_address(x.blas);
		auto instance = vk::AccelerationStructureInstanceKHR()
			.setTransform(xform)
			.setInstanceCustomIndex(x.index)
			.setMask(0xff)
			.setAccelerationStructureReference(addr);

		instances.push_back(instance);
	}

	auto span = std::span(instances);
	auto buffer = Buffer::from(
		*this,
		span.size_bytes(),
		vk::BufferUsageFlagBits::eShaderDeviceAddress
		| vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write(span);

	auto instances_data = vk::AccelerationStructureGeometryInstancesDataKHR()
		.setArrayOfPointers(false)
		.setData(get_address(buffer));

	auto geometry_data = vk::AccelerationStructureGeometryDataKHR()
		.setInstances(instances_data);

	auto geometry = vk::AccelerationStructureGeometryKHR()
		.setGeometryType(vk::GeometryTypeKHR::eInstances)
		.setGeometry(geometry_data);

	auto build_info = vk::AccelerationStructureBuildGeometryInfoKHR()
		.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
		.setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastBuild)
		.setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
		.setGeometries(geometry);

	auto [tlas, scratch_size] = AccelerationStructure::from(*this, build_info, instances.size());

	auto scratch = Buffer::from(*this,
		scratch_size,
		vk::BufferUsageFlagBits::eStorageBuffer
		| vk::BufferUsageFlagBits::eShaderDeviceAddress,
		vk::MemoryPropertyFlagBits::eDeviceLocal
	);

	build_info
		.setDstAccelerationStructure(tlas.handle)
		.setScratchData(get_address(scratch));

	auto range = vk::AccelerationStructureBuildRangeInfoKHR()
		.setPrimitiveCount(instances.size());

	cmd.buildAccelerationStructuresKHR(build_info, &range, loader);

	return std::tuple { tlas, scratch };
}

void AccelerationStructure::destroy(const Device &device)
{
	if (handle) {
		device.logical.destroyAccelerationStructureKHR(handle, nullptr, device.loader);
		handle = nullptr;
	}

	buffer.destroy();
}

} // namespace rcgp
