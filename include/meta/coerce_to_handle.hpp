#pragma once

#include "reconstruct_type.hpp"

namespace rcgp {

inline auto coerce_to_handle(jems::null value)
{
	return jems::handle();
}

template <builtin T>
auto coerce_to_handle(const T &value)
{
	return value;
}

template <user_defined T>
auto coerce_to_handle(const T &value)
{
	auto field_handler = [&] <size_t I> () {
		using field_t = typename T::fields::template get <I>;
		if constexpr (std::is_same_v <field_t, jems::null>) {
			return std::tuple <> ();
		} else {
			return std::tuple {
				coerce_to_handle(value
					.template _rcgp_get <I> ()
				)
			};
		}
	};

	auto args = constexpr_for(Is, T::field_count,
		return std::tuple_cat(
			field_handler.template operator() <Is> ()...
		)
	);

	return std::apply([&](auto &&... handles) {
		return jems::construct(reconstruct_type <T> (), handles...);
	}, args);
}

} // namespace rcgp
