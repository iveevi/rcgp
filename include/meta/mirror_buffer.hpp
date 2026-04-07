#pragma once

#include "../rhi/buffer.hpp"
#include "dynamic.hpp"
#include "layouts.hpp"

namespace rcgp {

template <typename T, template <typename> typename L>
struct BufferAddress {
	uint64_t value = 0;
};

// TODO: move to dynamic.hpp
template <typename T, template <typename> typename L>
using dynamic_element_of_mirror = typename std::decay_t <
	decltype(std::get <0> (dynamic_part <T> (std::declval <layouts::apply_t <T, L> &> ())))
> ::value_type;

template <typename T, template <typename> typename L, vk::BufferUsageFlagBits F>
struct MirrorBuffer : Buffer {};

template <non_dynamic T, template <typename> typename L, vk::BufferUsageFlagBits F>
struct MirrorBuffer <T, L, F> : Buffer {
	using symbolic_type = T;
	using value_type = layouts::apply_t <T, L>;

	auto &write(const value_type &data) const {
		Buffer::write(&data, sizeof(value_type), 0);
		return *this;
	}

	auto &read(value_type &data) const {
		Buffer::read((void *) &data, sizeof(value_type), 0);
		return *this;
	}
	
	template <typename U>
	auto &write_unsafe(std::span <U> memory, size_t offset = 0) const {
		Buffer::write <U> (memory, offset);
		return *this;
	}
	
	template <typename U>
	auto &write_unsafe(std::span <const U> memory, size_t offset = 0) const {
		Buffer::write <U> (memory, offset);
		return *this;
	}

	template <typename U>
	auto &read_unsafe(std::span <U> memory, size_t offset = 0) const {
		Buffer::read <U> (memory, offset);
		return *this;
	}

	// TODO: do we need this method here again?
	vk::DescriptorBufferInfo descriptor_info() const {
		return vk::DescriptorBufferInfo()
			.setBuffer(handle)
			.setOffset(offset)
			.setRange(sizeof(value_type));
	}

	static MirrorBuffer from(
		const Device &device,
		vk::MemoryPropertyFlags properties,
		vk::BufferUsageFlags extra_usage = vk::BufferUsageFlagBits(0)
	) {
		auto base = Buffer::from(device,
			sizeof(value_type),
			F | extra_usage,
			properties
		);

		MirrorBuffer result;
		Tas <Buffer &> (result) = base;
		return result;
	}
};

template <dynamic T, template <typename> typename L, vk::BufferUsageFlagBits F>
struct MirrorBuffer <T, L, F> : Buffer {
	using symbolic_type = T;
	using value_type = layouts::apply_t <T, L>;
	using element_type = dynamic_element_of_mirror <T, L>;

	auto &write(const value_type &data) const {
		// TODO: handle flushing if not host coherent
		// TODO: do this all in one mapped context
		auto [dyn, offset] = dynamic_part <T> (data);
		if (offset > 0)
			Buffer::write(&data, offset, 0);

		Buffer::write(dyn.data(), std::span(dyn).size_bytes(), offset);
		return *this;
	}

	auto &read(value_type &data) const {
		auto [dyn, offset] = dynamic_part <T> (data);
		if (offset > 0)
			Buffer::read((void *) &data, offset, 0);

		Buffer::read((void *) dyn.data(), std::span(dyn).size_bytes(), offset);
		return *this;
	}
	
	template <typename U>
	auto &write_unsafe(std::span <U> memory, size_t offset = 0) const {
		Buffer::write <U> (memory, offset);
		return *this;
	}
	
	template <typename U>
	auto &write_unsafe(std::span <const U> memory, size_t offset = 0) const {
		Buffer::write <U> (memory, offset);
		return *this;
	}

	template <typename U>
	auto &read_unsafe(std::span <U> memory, size_t offset = 0) const {
		Buffer::read <U> (memory, offset);
		return *this;
	}

	static MirrorBuffer from(
		const Device &device,
		size_t max_elements,
		vk::MemoryPropertyFlags properties,
		vk::BufferUsageFlags extra_usage = vk::BufferUsageFlagBits(0)
	) {
		// TODO: dynamic_offset() static constexpr method
		value_type data;
		auto [dyn, offset] = dynamic_part <T> (data);
		auto base = Buffer::from(device,
			offset + max_elements * sizeof(element_type),
			F | extra_usage,
			properties
		);

		MirrorBuffer result;
		Tas <Buffer &> (result) = base;
		return result;
	}

	vk::DescriptorBufferInfo descriptor_info() const {
		return vk::DescriptorBufferInfo()
			.setBuffer(handle)
			.setOffset(offset)
			.setRange(size);
	}
};

// Aliases for specific kinds of buffers
template <typename T, template <typename> typename L>
using VertexMirrorBuffer = MirrorBuffer <T, L, vk::BufferUsageFlagBits::eVertexBuffer>;

template <typename T, template <typename> typename L>
using IndexMirrorBuffer = MirrorBuffer <T, L, vk::BufferUsageFlagBits::eIndexBuffer>;

template <typename T, template <typename> typename L>
using UniformMirrorBuffer = MirrorBuffer <T, L, vk::BufferUsageFlagBits::eUniformBuffer>;

template <typename T, template <typename> typename L>
using StorageMirrorBuffer = MirrorBuffer <T, L, vk::BufferUsageFlagBits::eStorageBuffer>;

// Buffer referenced via device address (for BufferReference<T, L> shader types)
template <typename T, template <typename> typename L = layouts::scalar>
using ReferencedBuffer = MirrorBuffer <T, L, vk::BufferUsageFlagBits::eShaderDeviceAddress>;

// Type traits
TYPE_TRAIT(is_vertex_buffer);
	template <typename T, template <typename> typename L>
	TYPE_TRAIT_INCLUDES(is_vertex_buffer, VertexMirrorBuffer <T, L>);

TYPE_TRAIT(is_index_buffer);
	template <typename T, template <typename> typename L>
	TYPE_TRAIT_INCLUDES(is_index_buffer, IndexMirrorBuffer <T, L>);

// Deferred implementation of Device::address (declared in device.hpp)
template <typename T, template <typename> typename L, vk::BufferUsageFlagBits F>
requires (bool(F & vk::BufferUsageFlagBits::eShaderDeviceAddress))
auto Device::address(const MirrorBuffer <T, L, F> &buffer) const -> BufferAddress <T, L>
{
	return { get_address(buffer) };
}

} // namespace rcgp
