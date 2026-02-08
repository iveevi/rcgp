#pragma once

#include "contract.hpp"
#include "resources.hpp"
#include "symbolic_format.hpp"

namespace rcgp {

template <auto &... refs>
constexpr auto sequence_to_vertex_bindings(const Tlist <contract <refs>...> &in)
{
	auto desc = [] <typename T, template <typename> typename L, vk::VertexInputRate R>
	(const AttributeStream <T, L, R> &, uint32_t binding) {
		using M = layouts::apply_t <T, L>;
		return vk::VertexInputBindingDescription {
			.binding = binding,
			.stride = sizeof(M),
			.inputRate = R,
		};
	};

	if constexpr (sizeof...(refs) == 0) {
		return std::array <vk::VertexInputBindingDescription, 0> ();
	} else {
		return constexpr_for(Is, sizeof...(refs),
			return std::array { desc(refs, Is)... }
		);
	}
}

template <auto &... refs>
constexpr auto sequence_to_vertex_attributes(const Tlist <contract <refs>...> &in)
{
	auto desc = [] <typename T, template <typename> typename L, vk::VertexInputRate R>
	(const AttributeStream <T, L, R> &, uint32_t binding, uint32_t location) {
		return vk::VertexInputAttributeDescription {
			.location = location,
			.binding = binding,
			.format = symbolic_format <T> ::value,
			.offset = 0,
		};
	};

	if constexpr (sizeof...(refs) == 0) {
		return std::array <vk::VertexInputAttributeDescription, 0> ();
	} else {
		size_t location = 0;
		return constexpr_for(Is, sizeof...(refs),
			return std::array { desc(refs, Is, location++)... }
		);
	}
}

} // namespace rcgp
