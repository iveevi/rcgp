#pragma once

#include "../dsl/array.hpp"
#include "../util/array.hpp"
#include "../util/tlist.hpp"
#include "field_access.hpp"
#include "concepts.hpp"

namespace rcgp {

template <typename T>
struct is_dynamic_layout : std::false_type {};

template <typename T>
struct is_dynamic_layout <array <T, -1>> : std::true_type {};

template <typename List>
struct is_dynamic_fields;

template <typename ... Ts>
struct is_dynamic_fields <Tlist <Ts...>>
	: std::bool_constant <(is_dynamic_layout <Ts> ::value || ...)> {};

template <aggregate T>
struct is_dynamic_layout <T> : is_dynamic_fields <typename T::fields> {};

template <typename T>
constexpr bool is_dynamic_layout_v = is_dynamic_layout <T> ::value;

template <typename T>
constexpr bool is_dynamic_v = is_dynamic_layout_v <T>;

template <typename T>
constexpr bool is_static_v = !is_dynamic_layout_v <T>;

template <typename T>
struct field_trace_of_dynamic {};

template <typename List>
struct field_trace_of_dynamic_list;

template <typename ... Ts>
struct field_trace_of_dynamic_list <Tlist <Ts...>> {
	template <typename T, size_t ... Is>
	using trace = decltype([] {
		constexpr auto N = sizeof...(Ts);
		constexpr auto dynamics = std::array <bool, N> {
			is_dynamic_layout_v <Ts>...
		};

		constexpr auto idx = first_on(dynamics);

		using D = Ts...[idx];

		return typename field_trace_of_dynamic <D>
			::template trace <T, Is..., idx> ();
	} ());
};

template <aggregate T>
struct field_trace_of_dynamic <T>
	: field_trace_of_dynamic_list <typename T::fields> {};

template <typename T>
struct field_trace_of_dynamic <array <T, -1>> {
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

template <typename T>
requires is_dynamic_layout_v <T>
auto dynamic_part(const auto &value)
{
	using trace = field_trace_of_dynamic <T> ::template trace <T>;

	auto &dyn = field_trace_get(trace(), value);
	auto offset = field_trace_offset(trace(), value);

	return std::tuple <decltype(dyn), size_t> (dyn, offset);
}

} // namespace rcgp
