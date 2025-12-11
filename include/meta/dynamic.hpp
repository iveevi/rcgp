#pragma once

#include "../util/array.hpp"
#include "expand_reflection.hpp"
#include "reflection.hpp"
#include "static_access_chain.hpp"

template <typename T>
requires is_dynamic_reflection_v <T>
struct field_trace_of_dynamic {};

template <typename Original, typename ... Ts>
struct field_trace_of_dynamic <aggregate_reflection <Original, Ts...>> {
	template <typename T, size_t ... Is>
	using trace = decltype([] {
		constexpr auto N = sizeof...(Ts);
		constexpr auto dynamics = std::array <bool, N> {
			is_dynamic_reflection_v <Ts>...
		};

		constexpr auto idx = first_on(dynamics);

		using D = Ts...[idx];

		return typename field_trace_of_dynamic <D>
			::template trace <T, Is..., idx> ();
	} ());
};

template <typename T>
struct field_trace_of_dynamic <array_reflection <T, -1>> {
	template <typename U, size_t ... Is>
	using trace = field_trace <U, Is...>;
};

template <typename T, size_t I, size_t ... Is>
auto &field_trace_get(field_trace <T, I, Is...>, auto &value)
{
	auto &once = value.template get <I> ();

	using U = std::decay_t <decltype(once)>;
	if constexpr (sizeof...(Is))
		return field_trace_get(field_trace <U, Is...> (), once);
	else
		return once;
}

template <typename T>
auto &field_trace_get(field_trace <T>, auto &value)
{
	return value;
}

template <typename T, size_t I, size_t ... Is>
size_t field_trace_offset(field_trace <T, I, Is...>, auto &value)
{
	auto &once = value.template get <I> ();
	auto offset = value.template offset <I> ();

	using U = std::decay_t <decltype(once)>;
	if constexpr (sizeof...(Is))
		return offset + field_trace_offset(field_trace <U, Is...> (), once);
	else
		return offset;
}

template <typename T>
size_t field_trace_offset(field_trace <T>, auto &value)
{
	return 0;
}

template <reflected T>
requires is_dynamic_v <T>
auto dynamic_part(const auto &value)
{
	using R = expand_reflection_t <T>;
	using trace = field_trace_of_dynamic <R> ::template trace <R>;

	auto &dyn = field_trace_get(trace(), value);
	auto offset = field_trace_offset(trace(), value);

	return std::tuple <decltype(dyn), size_t> (dyn, offset);
}
