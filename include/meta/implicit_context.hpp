#pragma once

#include <functional>

#include "reference.hpp"

template <auto & ... refs>
struct implicit_context {};

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

template <auto & ... refs>
auto new_implicit_context_impl(implicit_context <refs...>) -> implicit_context <refs...>;

template <typename T, typename ... Args, auto &... refs>
auto new_implicit_context_impl(implicit_context <refs...> context, T *, Args *... args)
{
	if constexpr (is_reference <T> ::value)
		return new_implicit_context_impl <Args...> (implicit_context <refs ..., T::handle> (), args...);
	else
		return new_implicit_context_impl <Args...> (context, args...);
}

template <typename ... Args>
auto new_implicit_context(std::function <void (Args ...)>)
{
	return new_implicit_context_impl <Args...> (implicit_context <> (), (Args *) nullptr...);
}

#define $context_capture(...) (decltype(new_implicit_context(std::function([](__VA_ARGS__) {}))) _ref_context __VA_OPT__(,) __VA_ARGS__)
#define $context _ref_context
