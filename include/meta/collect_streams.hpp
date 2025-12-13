#pragma once

#include "implicit_context.hpp"
#include "resources.hpp"

template <auto &ref, typename ... Ts>
auto add_stream(const sequence <Ts...> &in)
{
	using base = reference <ref> ::raw_base;
	if constexpr (!is_attribute_stream_v <base>) {
		return in;
	} else {
		constexpr auto exists = (std::same_as <Ts, reference <ref>> || ...);
		if constexpr (exists)
			return in;
		else
			return sequence <Ts..., reference <ref>> ::singleton;
	}
}

template <typename ... Ts, auto &b, auto &... bs>
auto add_stream_from_implicit_context(const sequence <Ts...> &in, const implicit_context <b, bs...> &)
{
	auto out = add_stream <b> (in);
	if constexpr (sizeof...(bs))
		return add_stream_from_implicit_context(out, implicit_context <bs...> ());
	else
		return out;
}
