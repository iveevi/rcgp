#pragma once

#include <type_traits>

namespace fn_return_injection {

template <int>
struct proxy_tag {};

template <typename T>
struct foil {
	using unfoil = T;
};

template <typename T>
struct Reader {
	friend constexpr auto adl_lever(Reader <T>);
};

template <typename T, typename U>
struct Writer {
	friend constexpr auto adl_lever(Reader <T>) {
		return foil <U> {};
	}
};

inline void adl_lever();

template <typename T>
using Read = std::remove_pointer_t <decltype(adl_lever(Reader <T> {}))>;

}
