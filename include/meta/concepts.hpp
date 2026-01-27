#pragma once

#include "../dsl/matrix.hpp"
#include "../dsl/scalar.hpp"
#include "../dsl/vector.hpp"
#include "../util/cti.hpp"

namespace rcgp {

TYPE_TRAIT(is_primitive);
	template <typename T>
	TYPE_TRAIT_INCLUDES(is_primitive, scalar <T>);
	
	template <typename T, size_t N>
	TYPE_TRAIT_INCLUDES(is_primitive, vector <T, N>);
	
	template <typename T, size_t N, size_t M>
	TYPE_TRAIT_INCLUDES(is_primitive, matrix <T, N, M>);

template <typename T>
concept primitive = is_primitive_v <T>;

template <typename T>
concept aggregate = requires {
	typename T::_rcgp_aggregate;
} && std::is_same_v <
	typename T::_rcgp_aggregate,
	std::type_identity <T>
>;

} // namespace rcgp
