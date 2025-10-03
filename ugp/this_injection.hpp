#pragma once

#include <type_traits>

namespace this_injection {

template <typename T>
struct Reader {
	friend auto adl_lever(Reader <T>);
};

template <typename T, typename U>
struct Writer {
	friend auto adl_lever(Reader <T>) {
		return U {};
	}
};

inline void adl_lever();

template <typename T>
using Read = std::remove_pointer_t <decltype(adl_lever(Reader <T> {}))>;

};

#define THIS_INJECTION()				\
	struct _injection_tag {};			\
	constexpr auto _this_injector() -> decltype(	\
		this_injection::Writer <		\
			_injection_tag,			\
			decltype(this)			\
		> {}, void()				\
	);						\
	using This = this_injection::Read <_injection_tag>;
