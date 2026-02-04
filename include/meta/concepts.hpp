#pragma once

#include "../dsl/array.hpp"
#include "../dsl/matrix.hpp"
#include "../dsl/scalar.hpp"
#include "../dsl/vector.hpp"
#include "../util/tlist.hpp"
#include "../util/cti.hpp"
#include <type_traits>

namespace rcgp {

// Primitive types
template <typename T>
concept primitive = std::is_base_of_v <jems::handle, T>;

// User-defined types
template <typename T>
concept aggregate = requires {
	typename T::_rcgp_aggregate;
} && std::is_same_v <
	typename T::_rcgp_aggregate,
	std::type_identity <T>
>;

// Dynamic types
TYPE_TRAIT(is_dynamic);
	template <typename T>
	TYPE_TRAIT_INCLUDES(is_dynamic, array <T, -1>);

template <aggregate T>
struct is_dynamic <T> {
	static constexpr bool value = [] {
		return constexpr_for(Is, T::field_count,
			return (is_dynamic <
				typename T::fields::template get <Is>
			> ::value || ...)
		);
	} ();
};

template <typename T>
concept dynamic = is_dynamic_v <T>;

template <typename T>
concept non_dynamic = not is_dynamic_v <T>;

} // namespace rcgp
