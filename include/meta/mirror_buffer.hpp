#pragma once

#include "mirror.hpp"
#include "../rhi/buffer.hpp"

template <reflected T, template <typename> typename L>
using dynamic_element_of_mirror = decltype([] {
	TypeMirror <T, L> data;
	auto [dyn, offset] = dynamic_part <T> (data);
	return typename std::decay_t <decltype(dyn)> ::value_type();
} ());

template <reflected T, template <typename> typename L, vk::BufferUsageFlagBits F>
struct MirrorBuffer : Buffer {};

template <reflected T, template <typename> typename L, vk::BufferUsageFlagBits F>
requires (is_static_v <T>)
struct MirrorBuffer <T, L, F> : Buffer {
	using value_type = TypeMirror <T, L>;

	value_type new_value() const {
		return value_type();
	}

	auto &write(const value_type &data) const {
		Buffer::write(&data, sizeof(value_type), 0);
		return *this;
	}

	vk::DescriptorBufferInfo descriptor_info() const {
		return vk::DescriptorBufferInfo()
			.setBuffer(handle)
			.setOffset(offset)
			.setRange(sizeof(value_type));
	}

	static MirrorBuffer from(const Device &device,
			  	 vk::MemoryPropertyFlags properties,
			  	 vk::BufferUsageFlags extra_usage = vk::BufferUsageFlagBits(0)) {
		auto base = Buffer::from(device, sizeof(value_type), F | extra_usage, properties);

		MirrorBuffer result;
		static_cast <Buffer &> (result) = base;
		return result;
	}
};

template <reflected T, template <typename> typename L, vk::BufferUsageFlagBits F>
requires (is_dynamic_v <T>)
struct MirrorBuffer <T, L, F> : Buffer {
	using value_type = TypeMirror <T, L>;
	using element_type = dynamic_element_of_mirror <T, L>;
	
	value_type new_value() const {
		return value_type();
	}

	auto &write(const value_type &data) const {
		// TODO: do this all in one mapped context
		auto [dyn, offset] = dynamic_part <T> (data);
		if (offset > 0)
			Buffer::write(&data, offset, 0);

		Buffer::write(dyn.data(), std::span(dyn).size_bytes(), offset);
		return *this;
	}

	static MirrorBuffer from(const Device &device,
			  	 size_t max_elements,
			  	 vk::MemoryPropertyFlags properties,
			  	 vk::BufferUsageFlags extra_usage = vk::BufferUsageFlagBits(0)) {
		// TODO: dynamic_offset() static constexpr method
		value_type data;
		auto [dyn, offset] = dynamic_part <T> (data);
		auto base = Buffer::from(device, offset + max_elements * sizeof(element_type), F | extra_usage, properties);

		MirrorBuffer result;
		static_cast <Buffer &> (result) = base;
		return result;
	}
};

// Aliases for specific kinds of buffers
template <reflected T, template <typename> typename L>
using VertexMirrorBuffer = MirrorBuffer <T, L, vk::BufferUsageFlagBits::eVertexBuffer>;

template <reflected T, template <typename> typename L>
using IndexMirrorBuffer = MirrorBuffer <T, L, vk::BufferUsageFlagBits::eIndexBuffer>;

template <reflected T, template <typename> typename L>
using UniformMirrorBuffer = MirrorBuffer <T, L, vk::BufferUsageFlagBits::eUniformBuffer>;
