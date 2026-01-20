#pragma once

#include "../util/cti.hpp"
#include "reference.hpp"

template <auto &... refs>
struct implicit_context {};

TYPE_TRAIT(is_implicit_context);
	template <auto &... refs>
	TYPE_TRAIT_INCLUDES(is_implicit_context, implicit_context <refs...>);

template <auto &... refs1, auto &... refs2, typename ... Rest>
consteval auto icontext_concat(implicit_context <refs1...>, implicit_context <refs2...>, Rest... rest)
{
	auto interim = implicit_context <refs1..., refs2...> {};
	if constexpr (sizeof...(Rest))
		return icontext_concat(interim, rest...);
	else
		return interim;
}

template <typename T>
using atomic_icontext = decltype([] {
	if constexpr (is_implicit_context_v <T>)
		return T();
	else if constexpr (is_reference_v <T>)
		return implicit_context <T::handle> ();
	else
		return implicit_context <> ();
} ());

template <typename ... Ts>
consteval auto icontext_from_args(Ts &&... args)
{
	return icontext_concat(
		implicit_context <> (),
		implicit_context <> (),
		atomic_icontext <Ts> ()...
	);
}

template <typename ... Ts>
consteval auto icontext_from_vptr(void (*)(Ts ...))
{
	return icontext_concat(
		implicit_context <> (),
		implicit_context <> (),
		atomic_icontext <Ts> ()...
	);
}

template <typename ... Ts>
using icontext_from_args_t = decltype(icontext_from_args(Ts()...));
