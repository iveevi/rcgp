#pragma once

#include "../rhi/buffer.hpp"
#include "dynamic.hpp"
#include "mirror.hpp"
#include "resources.hpp"

namespace rcgp {

template <typename T, template <typename> typename L>
using dynamic_element_of_mirror = decltype([] {
	TypeMirror <T, L> data;
	auto [dyn, offset] = dynamic_part <T> (data);
	return typename std::decay_t <decltype(dyn)> ::value_type();
} ());

template <typename T, template <typename> typename L, vk::BufferUsageFlagBits F>
struct MirrorBuffer : Buffer {};

template <typename T, template <typename> typename L, vk::BufferUsageFlagBits F>
requires (is_static_v <T>)
struct MirrorBuffer <T, L, F> : Buffer {
	using symbolic_type = T;
	using value_type = TypeMirror <T, L>;

	value_type new_value() const {
		return value_type();
	}

	auto &write(const value_type &data) const {
		Buffer::write(&data, sizeof(value_type), 0);
		return *this;
	}

	auto &read(value_type &data) const {
		Buffer::read((void *) &data, sizeof(value_type), 0);
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
		Tas <Buffer &> (result) = base;
		return result;
	}
};

template <typename T, template <typename> typename L, vk::BufferUsageFlagBits F>
requires (is_dynamic_v <T>)
struct MirrorBuffer <T, L, F> : Buffer {
	using symbolic_type = T;
	using value_type = TypeMirror <T, L>;
	using element_type = dynamic_element_of_mirror <T, L>;
	
	value_type new_value() const {
		return value_type();
	}

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
	auto &write_unsafe(std::span <const U> memory, size_t offset = 0) const {
		Buffer::write <U> (memory, offset);
		return *this;
	}

	template <typename U>
	auto &read_unsafe(std::span <U> memory, size_t offset = 0) const {
		Buffer::read <U> (memory, offset);
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

// Now we can add some specializations for resource translation
template <typename T, template <typename> typename L>
struct resource_translator <UniformBuffer <T, L>> {
	using type = UniformMirrorBuffer <T, L>;
	using value_type = type::value_type;
	using element_type = std::nullptr_t;
};

template <typename T, template <typename> typename L, GlobalResourceAccess A>
struct resource_translator <StorageBuffer <T, L, A>> {
	using type = StorageMirrorBuffer <T, L>;
	using value_type = type::value_type;
	using element_type = std::nullptr_t;
};

} // namespace rcgp
