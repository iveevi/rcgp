#pragma once

#include "../util/array.hpp"
#include "reference.hpp"
#include "reflection.hpp"
#include "resources.hpp"
#include "symbolic_format.hpp"

// TODO: handle aggregates and such
template <reflected T, template <typename> typename L, vk::VertexInputRate R>
auto attribute_description_for_attribute_stream(const AttributeStream <T, L, R> &, size_t binding, size_t &location)
{
	return std::array {
		vk::VertexInputAttributeDescription()
			.setLocation(location++)
			.setBinding(binding)
			.setFormat(symbolic_format <T> ::value)
			.setOffset(0)
	};
}

template <auto &... refs, size_t ... Is>
auto sequence_to_vertex_attributes_impl(const sequence <reference <refs>...> &, const std::index_sequence <Is...> &)
{
	size_t location = 0;
	return concat(attribute_description_for_attribute_stream(refs, Is, location)...);
}

template <auto &... refs>
auto sequence_to_vertex_attributes(const sequence <reference <refs>...> &in)
{
	if constexpr (sizeof...(refs) == 0)
		return std::array <vk::VertexInputAttributeDescription, 0> ();
	else
		return sequence_to_vertex_attributes_impl(in, std::make_index_sequence <sizeof...(refs)> ());
}
