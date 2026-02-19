#pragma once

#include <variant>
#include <optional>

namespace rcgp {

template <typename T, typename U, typename ... Args>
constexpr int variant_index(int i)
{
	if constexpr (std::same_as <T, U>)
		return i;
	if constexpr (sizeof...(Args))
		return variant_index <T, Args...> (i + 1);
	return -1;
}

template <typename ... Args>
struct variant : std::variant <Args...> {
	using std::variant <Args...> ::variant;

	using variant_self = variant <Args...>;

	template <typename T>
	bool is() const {
		return std::holds_alternative <T> (*this);
	}
	
	template <typename T>
	T &as() {
		return std::get <T> (*this);
	}
	
	template <typename T>
	const T &as() const {
		return std::get <T> (*this);
	}
	
	template <typename T>
	std::optional <T> maybe() {
		if (is <T> ())
			return as <T> ();
		else
			return std::nullopt;
	}

	template <typename T>
	static constexpr int type_index() {
		return variant_index <T, Args...> (0);
	}
};

#define vswitch(value)					\
	using T = std::decay_t <decltype(value)>;	\
	switch ((value).index())

#define vcase(...) case T::type_index <__VA_ARGS__> ()

} // namespace rcgp
