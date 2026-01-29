#pragma once

#include <tuple>
#include <type_traits>

#include "../util/cti.hpp"
#include "../util/tlist.hpp"
#include "concepts.hpp"
#include "stage_intrinsics.hpp"
#include "static_string.hpp"

namespace rcgp {

namespace detail {

template <typename T>
struct vertex_output_value {
	using type = T;
};

template <primitive T, RateProperties P>
struct vertex_output_value <Interpolant <T, P>> {
	using type = T;
};

template <typename T>
using vertex_output_value_t = typename vertex_output_value <T> ::type;

template <typename T>
struct vertex_output_list {
	using type = Tlist <vertex_output_value_t <T>>;
};

template <>
struct vertex_output_list <void> {
	using type = Tlist <>;
};

template <typename ... Ts>
struct vertex_output_list <std::tuple <Ts...>> {
	using type = tlist_concat_t <typename vertex_output_list <Ts> ::type...>;
};

template <typename T>
struct is_varying_input
	: std::bool_constant <primitive <T> || aggregate <T>> {};

template <typename ... Ts>
using fragment_input_list_t = tlist_filter_t <is_varying_input, Tlist <Ts...>>;

template <typename Outputs, typename Inputs, size_t I>
constexpr void validate_interface_types()
{
	if constexpr (I < Inputs::size && I < Outputs::size) {
		using OutT = typename Outputs::template get <I>;
		using InT = typename Inputs::template get <I>;
		if constexpr (not std::is_same_v <OutT, InT>) {
			static_error(
				"vertex/fragment interface mismatch at location "_ss
				+ $ss_ulong_decimal(I)
				+ ": vertex outputs "_ss
				+ $ss_type(OutT)
				+ " but fragment expects "_ss
				+ $ss_type(InT)
			);
		}
		validate_interface_types <Outputs, Inputs, I + 1> ();
	}
}

template <typename Outputs, typename Inputs>
constexpr bool validate_vertex_fragment_interface()
{
	if constexpr (Outputs::size != Inputs::size) {
		static_error(
			"vertex/fragment interface mismatch: fragment expects "_ss
			+ $ss_ulong_decimal(Inputs::size)
			+ " inputs but vertex outputs "_ss
			+ $ss_ulong_decimal(Outputs::size)
		);
	}

	validate_interface_types <Outputs, Inputs, 0> ();
	return true;
}

} // namespace detail

template <typename VRet, typename ... FArgs>
struct vertex_fragment_interface {
	using outputs = typename detail::vertex_output_list <VRet> ::type;
	using inputs = detail::fragment_input_list_t <FArgs...>;
	static constexpr bool value = detail::validate_vertex_fragment_interface <outputs, inputs> ();
};

} // namespace rcgp
