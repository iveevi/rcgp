#pragma once

#include <cstdlib>
#include <type_traits>

namespace rcgp {

// Tuples without the baggage
template <typename ... Args>
struct Tlist {
	static constexpr size_t size = sizeof...(Args);

	template <size_t I>
	using get = decltype([] {
		// GCC doesn't like indexing empty packs :(
		if constexpr (I < sizeof...(Args))
			return (Args...[I]) {};
		else
			return int();
	} ());

	template <typename T>
	using insert = Tlist <T, Args...>;

	template <template <typename ...> typename F>
	using invoke = F <Args...>;
};

// Tlist concatenation
template <typename ... Ts, typename ... Us, typename ... Rest>
consteval auto tlist_concat(Tlist <Ts...>, Tlist <Us...>, Rest... rest)
{
	auto interim = Tlist <Ts..., Us...> {};
	if constexpr (sizeof...(Rest))
		return tlist_concat(interim, rest...);
	else
		return interim;
}

// Tlist filtering with arbitrary conditions
template <template <typename> typename Filter, typename ... Ts>
auto tlist_filter(Tlist <Ts...> ongoing, Tlist <> processing)
{
	return ongoing;
}

template <template <typename> typename Filter, typename ... Ts, typename C, typename ... Rest>
auto tlist_filter(Tlist <Ts...> ongoing, Tlist <C, Rest...> processing)
{
	constexpr auto rest = Tlist <Rest...> {};
	if constexpr (Filter <C> ::value)
		return tlist_filter <Filter> (Tlist <Ts..., C> {}, rest);
	else
		return tlist_filter <Filter> (ongoing, rest);
}

template <template <typename> typename Filter, typename List>
using tlist_filter_t = decltype(tlist_filter <Filter> (Tlist <> {}, List {}));

template <typename T, typename List>
struct tlist_contains;

template <typename T>
struct tlist_contains <T, Tlist <>> : std::false_type {};

template <typename T, typename Head, typename ... Rest>
struct tlist_contains <T, Tlist <Head, Rest...>>
	: std::bool_constant <
		std::is_same_v <T, Head> or tlist_contains <T, Tlist <Rest...>> ::value
	> {};

template <typename List>
struct tlist_unique;

template <>
struct tlist_unique <Tlist <>> {
	using type = Tlist <>;
};

template <typename Head, typename ... Rest>
struct tlist_unique <Tlist <Head, Rest...>> {
	using rest = typename tlist_unique <Tlist <Rest...>> ::type;
	using type = std::conditional_t <
		tlist_contains <Head, rest> ::value,
		rest,
		typename rest::template insert <Head>
	>;
};

template <typename List>
using tlist_unique_t = typename tlist_unique <List> ::type;

template <typename List, template <typename> typename Transform>
struct tlist_transform;

template <template <typename> typename Transform, typename ... Ts>
struct tlist_transform <Tlist <Ts...>, Transform> {
	using type = Tlist <typename Transform <Ts> ::type...>;
};

template <typename List, template <typename> typename Transform>
using tlist_transform_t = typename tlist_transform <List, Transform> ::type;

template <typename ... Lists>
using tlist_concat_t = decltype(tlist_concat(Lists {}...));

} // namespace rcgp
