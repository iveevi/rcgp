#pragma once

#include "mirror_buffer.hpp"

// Reference to vertex buffer
template <auto &ref>
struct VertexBufferOf {
	static_assert(false, "not a stream");
};

template <reflected T, template <typename> typename L, vk::VertexInputRate R, AttributeStream <T, L, R> &ref>
struct VertexBufferOf  <ref> : VertexMirrorBuffer <array <T>, L> {
	using typename VertexMirrorBuffer <array <T>, L> ::MirrorBuffer;

	VertexBufferOf() = default;
	VertexBufferOf(const VertexMirrorBuffer <array <T>, L> &other)
		: VertexMirrorBuffer <array <T>, L> (other) {}

	auto &write(const typename VertexMirrorBuffer <array <T>, L> ::value_type &data) const {
		VertexMirrorBuffer <array <T>, L> ::write(data);
		return *this;
	}
	
	static VertexBufferOf from(const Device &device,
			  	 size_t max_elements,
			  	 vk::MemoryPropertyFlags properties,
			  	 vk::BufferUsageFlags extra_usage = vk::BufferUsageFlagBits(0)) {
		return VertexMirrorBuffer <array <T>, L> ::from(device, max_elements, properties, extra_usage);
	}
};
