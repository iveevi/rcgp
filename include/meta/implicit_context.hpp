#pragma once

#include <array>
#include <functional>

#include "../util/array.hpp"
#include "reference.hpp"

template <auto & ... refs>
struct implicit_context {
	template <auto &ref>
	using append = implicit_context <refs..., ref>;
};

template <typename T>
struct is_implicit_context : std::false_type {};

template <auto & ... refs>
struct is_implicit_context <implicit_context <refs...>> : std::true_type {};

template <typename T>
constexpr bool is_implicit_context_v = is_implicit_context <T> ::value;

template <typename ... Ts>
struct find_implicit_context {
	static constexpr auto contexts = std::array { is_implicit_context_v <Ts>... };
	static constexpr auto idx = first_on(contexts);
	using type = Ts...[idx];
};

template <typename Ctx, typename ... Args>
struct filter_into_context {
	using type = Ctx;
};

template <auto &... refs, auto &ref, typename ... Args>
struct filter_into_context <implicit_context <refs...>, reference <ref>, Args...> {
	using next = typename implicit_context <refs...> ::template append <ref>;
	using type = typename filter_into_context <next, Args...> ::type;
};

template <typename Ctx, typename Arg, typename ... Args>
struct filter_into_context <Ctx, Arg, Args...> {
	using type = typename filter_into_context <Ctx, Args...> ::type;
};

template <typename ... Args>
auto new_implicit_context(std::function <void (Args ...)>)
{
	using type = typename filter_into_context <implicit_context <>, Args...> ::type;
	return type();
}

#define $context_capture(...) (decltype(new_implicit_context(std::function([](__VA_ARGS__) {}))) _ref_context __VA_OPT__(,) __VA_ARGS__)
#define $context _ref_context
