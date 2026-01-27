#pragma once

#include "resources.hpp"

// TODO: remove this file...
namespace rcgp {

template <typename T, size_t ... Is>
struct field_trace {
	using value_type = T;
};

template <size_t I, size_t ... Is, aggregate T>
auto &field_access(T &value)
{
	auto &one = value.template _ugp_field_reference <I> ();
	if constexpr (sizeof...(Is))
		return field_access <Is...> (one);
	else
		return one;
}

template <size_t I, size_t ... Is, aggregate T>
const auto &field_access(const T &value)
{
	const auto &one = value.template _ugp_field_reference <I> ();
	if constexpr (sizeof...(Is))
		return field_access <Is...> (one);
	else
		return one;
}

template <size_t I, size_t ... Is, aggregate T>
auto &field_access(ResourceGroup <T> &value)
{
	auto &one = value.template _ugp_field_reference <I> ();
	if constexpr (sizeof...(Is))
		return field_access <Is...> (one);
	else
		return one;
}

template <size_t I, size_t ... Is, aggregate T>
const auto &field_access(const ResourceGroup <T> &value)
{
	const auto &one = value.template _ugp_field_reference <I> ();
	if constexpr (sizeof...(Is))
		return field_access <Is...> (one);
	else
		return one;
}

template <typename T>
auto &field_access(T &value)
{
	return value;
}

} // namespace rcgp
