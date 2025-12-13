#pragma once

#include "../dsl/primitives.hpp"
#include "mirror_buffer.hpp"
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

	VertexBufferOf(const VertexMirrorBuffer <array <T>, L> &other)
		: VertexMirrorBuffer <array <T>, L> (other) {}

	auto &write(const typename VertexMirrorBuffer <array <T>, L> ::value_type &data) const {
		VertexMirrorBuffer <array <T>, L> ::write(data);
		return *this;
	}
	
	static VertexBufferOf from(const Device &device,
			  	 size_t max_elements,
			  	 vk::MemoryPropertyFlags properties,
			  	 vk::BufferUsageFlags extra_usage = vk::BufferUsageFlagBits(0))
	{
		return VertexMirrorBuffer <array <T>, L> ::from(device, max_elements, properties, extra_usage);
	}
};

// Reference to resource handle
template <auto &ref>
struct resource_for_reference {
	static_assert(false, "not a resource group");
	using type = std::nullptr_t;
};

template <typename T>
struct resource_for_reference_impl {
	static_assert(false, ($ss("resource for reference (impl) not implemented for type ") + $ss_type(T)).view());
	using type = std::nullptr_t;
};

// TODO: aggregates become corresponding mirrors...

template <reflected T>
struct resource_for_reference_impl <ConstantBuffer <T>> {
	// TODO: buffers need layouts...
	using type = UniformMirrorBuffer <T, layouts::std430>;
};

template <reflected T, ResourceGroup <T> &ref>
struct resource_for_reference <ref> {
	// TODO: avoid this impl stuff; static cast?
	using type = resource_for_reference_impl <T> ::type; 
};

// TODO: lower to raw_base of reference...
template <auto &ref>
using ResourceOf = resource_for_reference <ref> ::type;

// Reference to type mirror of contents (if its a buffer or stream)
template <reflected T>
struct type_mirror_for_resource {
	static_assert(false, ($ss("type mirror not implemented for resource of type ") + $ss_type(T)).view());
	using type = std::nullptr_t;
	using element = std::nullptr_t;
};

template <reflected T, template <typename> typename L, vk::VertexInputRate R>
struct type_mirror_for_resource <AttributeStream <T, L, R>> {
	using type = TypeMirror <array <T>, L>;
	using element = TypeMirror <T, L>;
};

template <reflected T>
struct type_mirror_for_resource <ConstantBuffer <T>> {
	using type = TypeMirror <T, layouts::std430>;
	using element = std::nullptr_t;
};

template <reflected T>
struct type_mirror_for_resource <ResourceGroup <T>> {
	using type = type_mirror_for_resource <T> ::type;
	using element = type_mirror_for_resource <T> ::element;
};

template <auto &ref>
using MirrorOf = type_mirror_for_resource <
	// TODO: reference_base_t
	std::remove_reference_t <decltype(ref)>
> ::type;

// Reference to dynamic element of type mirror of contents (if its a buffer or streams)
template <auto &ref>
using DynamicElementOf = decltype([] {
	using T = std::remove_reference_t <decltype(ref)>;
	using E = typename type_mirror_for_resource <T> ::element;
	if constexpr (std::same_as <E, std::nullptr_t>)
		static_assert(false, "mirror is not dynamic");
	return E();
} ());
