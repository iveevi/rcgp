#pragma once

#include "../dsl/array.hpp"
#include "../util/array.hpp"
#include "../util/tlist.hpp"
#include <utility>
#include "concepts.hpp"

namespace rcgp {

template <typename Seq, size_t I>
struct index_sequence_append;

template <size_t ... Is, size_t I>
struct index_sequence_append <std::index_sequence <Is...>, I>
{
	using type = std::index_sequence <Is..., I>;
};

template <typename T, typename Seq = std::index_sequence <>>
struct dynamic_path_of {};

template <typename List, typename Seq>
struct dynamic_path_of_list;

template <typename ... Ts, typename Seq>
struct dynamic_path_of_list <Tlist <Ts...>, Seq> {
	using path = decltype([] {
		constexpr auto N = sizeof...(Ts);
		constexpr auto dynamics = std::array <bool, N> {
			is_dynamic_v <Ts>...
		};

		constexpr auto idx = first_on(dynamics);

		using D = Ts...[idx];
		using next = typename index_sequence_append <Seq, idx> ::type;

		return typename dynamic_path_of <D, next> ::path();
	} ());
};

template <aggregate T, typename Seq>
struct dynamic_path_of <T, Seq>
	: dynamic_path_of_list <typename T::fields, Seq> {};

template <typename T, typename Seq>
struct dynamic_path_of <array <T, -1>, Seq> {
	using path = Seq;
};

template <typename T>
auto &dynamic_get(std::index_sequence <>, T &value)
{
	return value;
}

template <typename T, size_t I, size_t ... Is>
auto &dynamic_get(std::index_sequence <I, Is...>, T &value)
{
	auto &once = value.template get <I> ();

	if constexpr (sizeof...(Is))
		return dynamic_get(std::index_sequence <Is...> (), once);
	else
		return once;
}

template <typename T>
size_t dynamic_offset(std::index_sequence <>, T &)
{
	return 0;
}

template <typename T, size_t I, size_t ... Is>
size_t dynamic_offset(std::index_sequence <I, Is...>, T &value)
{
	auto &once = value.template get <I> ();
	auto offset = value.template offset <I> ();

	if constexpr (sizeof...(Is))
		return offset + dynamic_offset(std::index_sequence <Is...> (), once);
	else
		return offset;
}

template <dynamic T>
auto dynamic_part(const auto &value)
{
	using path = dynamic_path_of <T> ::path;

	auto &dyn = dynamic_get(path(), value);
	auto offset = dynamic_offset(path(), value);

	return std::tuple <decltype(dyn), size_t> (dyn, offset);
}

} // namespace rcgp
