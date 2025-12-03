#pragma once

#include <cstdlib>

namespace field_name_injection {

template <typename T, size_t I>
struct field_ref {};

template <typename T>
struct Reader {
	friend constexpr auto adl_lever(Reader <T>);
};

template <typename T, auto Name>
struct Writer {
	friend constexpr auto adl_lever(Reader <T>) {
		return Name;
	}
};

void adl_lever();

} // namespace field_name_injection

#define FIELD_NAME_INJECTION(T, field)					\
	inline static const field_name_injection::Writer <		\
		field_name_injection::field_ref <			\
			T, decltype(field_counter::field)::value	\
		>, $ss(#field)						\
	> _##field##_name_injector;

#define $field_name(T, index) 						\
	adl_lever(				\
		field_name_injection::Reader <				\
			field_name_injection::field_ref <T, index>	\
		> {}							\
	)
