#pragma once

#include <algorithm>
#include <array>

#include "common.hpp"

template <typename ... Ts>
consteval auto std430_layout_engine_impl()
{
	constexpr size_t N = sizeof...(Ts);

	constexpr auto sizes = std::array <size_t, N> { sizeof(Ts)... };
	constexpr auto aligns = std::array <size_t, N> { alignment_v <Ts> ... };

	std::array <size_t, N> padding {};

	size_t offset = 0;
	size_t malign = 0;

	for (size_t i = 0; i < N; i++) {
		auto corrected = align_up(offset, aligns[i]);

		if (i > 0)
			padding[i - 1] = corrected - offset;

		offset = corrected + sizes[i];
		malign = std::max(malign, aligns[i]);
	}

	padding[N - 1] = align_up(offset, malign) - offset;

	return padding;
}

template <typename ... Ts>
consteval auto std430_layout_engine_dispatch(sequence <Ts...>)
{
	static constexpr auto padding = std430_layout_engine_impl <Ts...> ();

	using field_seq = sequence <Ts...>;
	using padding_seq = array_to_index_sequence <padding>;

	using stitched = layout_stitcher <field_seq, padding_seq> ::type;

	return typename stitched::trivial_tuple();
}

template <typename ... Ts>
consteval auto std430_layout_engine(sequence <Ts...> seq)
{
	using fseq = decltype(fix_alignment(seq));
	using tuple = decltype(std430_layout_engine_dispatch(std::declval <fseq> ()));
	return tuple();
}

template <typename ... Ts>
using std430_layout_t = decltype(std430_layout_engine(std::declval <sequence <Ts...>> ()));
