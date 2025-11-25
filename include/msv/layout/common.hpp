#pragma once

#include <array>
#include <cstddef>
#include <utility>

#include "../alignment.hpp"
#include "../../util/meta.hpp"

template <typename T, size_t N>
struct padded_t {
	T value;
	[[no_unique_address]] char _pad[N];

	padded_t &operator=(const T &value_) {
		value = value_;
		return *this;
	}

	operator T &() & {
		return value;
	}

	operator const T &() const {
		return value;
	}
};

template <typename T, size_t N>
struct alignment <padded_t <T, N>> {
	static constexpr size_t value = alignment <T> ::value;
};

template <auto &A, std::size_t ... Is>
constexpr auto array_to_index_sequence_impl(std::index_sequence <Is...>)
{
	return std::index_sequence <A[Is]...> {};
}

template <auto &A>
using array_to_index_sequence = decltype(array_to_index_sequence_impl <A> (std::make_index_sequence <A.size()> {}));

template <typename T>
struct fix_alignment_impl {
	using type = T;
};

template <typename T, size_t N>
struct fix_alignment_impl <T[N]> {
	static constexpr size_t padding = align_up(sizeof(T), alignment_v <T>) - sizeof(T);
	using base = padded_t <T, padding>;
	using type = base[N];
};

template <typename T>
struct fix_alignment_impl <T[]> {
	static constexpr size_t padding = align_up(sizeof(T), alignment_v <T>) - sizeof(T);
	using base = padded_t <T, padding>;
	using type = base[];
};

template <typename ... Ts>
consteval auto fix_alignment(sequence <Ts...>) -> sequence <typename fix_alignment_impl <Ts> ::type...>;

template <typename Fields, typename Padding>
struct layout_stitcher {
	using type = sequence <>;
};

template <typename T, typename ... Ts, size_t I, size_t ... Is>
struct layout_stitcher <sequence <T, Ts...>, std::index_sequence <I, Is...>> {
	using next = layout_stitcher <sequence <Ts...>, std::index_sequence <Is...>>;
	using type = next::type::template push_front_t <padded_t <T, I>>;
};
