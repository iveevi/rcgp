#pragma once

#include <type_traits>

#include "../dsl/jems.hpp"
#include "resources.hpp"
#include "static_string.hpp"

namespace rcgp {

// DSL handles
template <typename T>
requires std::is_base_of_v <jems::handle, T>
void inject_reference(T &value, Reference ref)
{
	value.override_reference(ref);
}

// User-defined types
template <aggregate T>
void inject_reference(T &value, Reference ref)
{
	size_t field_counter = 0;

	auto field_handler = [&] <size_t I> () {
		using field_t = typename T::fields::template get <I>;
		if constexpr (not std::is_same_v <field_t, std::nullptr_t>) {
			inject_reference(
				value.template _rcgp_get <I> (),
				jems::field_access(ref, field_counter++)
			);
		}
	};

	constexpr_for(Is, T::field_count,
		(field_handler.template operator() <Is> (), ...)
	);
}

// TODO: this applies to all buffer handles...
template <typename T, template <typename> typename L>
void inject_reference(PushConstant <T, L> &value, Reference ref)
{
	return inject_reference(Tas <T &> (value), ref);
}

template <typename T, template <typename> typename L, GlobalResourceAccess A>
void inject_reference(StorageBuffer <T, L, A> &value, Reference ref)
{
	return inject_reference(Tas <T &> (value), ref);
}

template <typename T, template <typename> typename L>
void inject_reference(UniformBuffer <T, L> &value, Reference ref)
{
	return inject_reference(Tas <T &> (value), ref);
}

// Fallback with error reported
template <typename T>
void inject_reference(T &value, Reference ref)
{
	static_error("failed to inject reference into item of type "_ss + $ss_type(T));
}

} // namespace rcgp
