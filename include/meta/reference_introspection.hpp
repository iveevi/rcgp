#pragma once

#include "../dsl/primitives.hpp"
#include "mirror_buffer.hpp"
#include "reference.hpp"
#include "reflection.hpp"
#include "resources.hpp"

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

// Reference to resource handle
// TODO: specialization for aggregate
template <reflected T>
struct resource_translator <ResourceGroup <T>> {
	using defer = resource_translator <T>;
	using type = defer::type;
	using value_type = defer::value_type;
	using element_type = defer::element_type;
};

template <reflected T, template <typename> typename L, vk::VertexInputRate R>
struct resource_translator <AttributeStream <T, L, R>> {
	using buffer = VertexMirrorBuffer <array <T>, L>;
	using type [[deprecated(
		"advised to use VertexBufferOf <>"
	)]] = decltype([]{
		return buffer();
	} ());
	using value_type = buffer::value_type;
	using element_type = buffer::element_type;
};

template <auto &rsrc>
using ResourceMirrorOf = ResourceMirror <reference_base_t <rsrc>>;

template <auto &rsrc>
using DataTypeOf = DataType <reference_base_t <rsrc>>;

template <auto &rsrc>
using DynamicDataTypeOf = DynamicDataType <reference_base_t <rsrc>>;
