#pragma once

#include "../util/cti.hpp"
#include "../util/tlist.hpp"
#include "concepts.hpp"
#include "static_string.hpp"

namespace rcgp {

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wattributes"
#endif

// Forward declarations
template <typename T, int64_t N>
struct array;

template <typename T>
using unsized_array = std::vector <T>;

// Pair of type and manual alignment
template <typename T, size_t N>
struct scaffold_hint {
	using type = T;
	static constexpr size_t value = N;
};

template <typename T>
using scaffold_natural = scaffold_hint <T, 0>;

// Scaffold for primitive/built-in types
template <size_t Align, typename T, bool AlignTopLevel = false>
struct alignas(AlignTopLevel ? Align : 0) scaffold_fundamental {
	T value;

	constexpr scaffold_fundamental() = default;

	template <typename U>
	requires (std::is_trivially_constructible_v <T, U>)
	constexpr scaffold_fundamental(const U &value_) : value(value_) {}

	operator const T &() const {
		return value;
	}
	
	operator T &() {
		return value;
	}
};

// Scaffold for structual types with fields
template <size_t Align, typename T, bool AlignTopLevel = false>
struct alignas(AlignTopLevel ? Align : 0) scaffold_structural : T {
	constexpr scaffold_structural() = default;

	template <typename U>
	requires (std::is_convertible_v <U, T>)
	constexpr scaffold_structural(const U &rhs) : T(rhs) {}

	template <typename U>
	requires (std::is_convertible_v <U, T>)
	constexpr scaffold_structural &operator=(const U &rhs) {
		T::operator=(rhs);
		return *this;
	}
};

// Scaffold lookup procedure
template <typename Hint, typename View, bool AlignTopLevel = false>
struct scaffold_lookup {
	static_assert(false, "no scaffold_lookup registered for hint '"_ss
		+ $ss_type(Hint) + "' and view '"_ss
		+ $ss_type(View) + "'... probably missing a scaffold_hint?"_ss);
};

// Primitive/built-in types
template <typename Mapped, size_t Align, typename View, bool AlignTopLevel>
requires (std::is_fundamental_v <Mapped>)
struct scaffold_lookup <scaffold_hint <Mapped, Align>, View, AlignTopLevel> {
	using type = std::conditional_t <
		Align == 0 or (sizeof(Mapped) % Align) == 0,
		Mapped,
		scaffold_fundamental <Align, Mapped, AlignTopLevel>
	>;
};

// Structural types
template <typename Mapped, size_t Align, typename View, bool AlignTopLevel>
struct scaffold_lookup <scaffold_hint <Mapped, Align>, View, AlignTopLevel> {
	using type = std::conditional_t <
		Align == 0 or (sizeof(Mapped) % Align) == 0,
		Mapped,
		scaffold_structural <Align, Mapped, AlignTopLevel>
	>;
};

// User-defined types
template <typename ... Ts, size_t Align, user_defined View, bool AlignTopLevel>
struct scaffold_lookup <scaffold_hint <Tlist <Ts...>, Align>, View, AlignTopLevel> {
	using type = View::template scaffold <Align, AlignTopLevel, Ts...>;
};

// Statically sized array types
template <typename Element, size_t N1, size_t Align, typename ElementView, int64_t N2, bool AlignTopLevel>
struct scaffold_lookup <
	scaffold_hint <std::array <Element, N1>, Align>,
	array <ElementView, N2>,
	AlignTopLevel
>
{
	static_assert(N1 == N2, "sizes are different");
	using element = scaffold_lookup <Element, ElementView, true> ::type;
        using type = std::array <element, N1>;
};

// Dynamically sized array types
template <typename Element, size_t Align, typename ElementView, bool AlignTopLevel>
struct scaffold_lookup <
	scaffold_hint <unsized_array <Element>, Align>,
	array <ElementView, -1>,
	AlignTopLevel
>
{
	using element = scaffold_lookup <Element, ElementView, true> ::type;
	// NOTE: AlignTopLevel is ignored because this is a dynamic element
	using type = unsized_array <element>;
};

// Generating the scaffold in the reflection building process
#define GEN_SCAFFOLD_FIELDS(T, field)						\
	using hint_##field = Ts...[__COUNTER__ - counter_base - 1];		\
	alignas(hint_##field::value) scaffold_lookup				\
		<hint_##field, decltype(T::field), false> ::type field;

#define GEN_SCAFFOLD_FIELD_GET(T, field)					\
	else if constexpr (D == ((__COUNTER__ - 1) - counter_base)) {		\
		return field;							\
	}

#define GEN_SCAFFOLD_FIELD_OFFSET(T, field)					\
	else if constexpr (I == ((__COUNTER__ - 1) - counter_base)) {		\
		return offsetof(scaffold, field);				\
	}

#define DEFINE_SCAFFOLD_GET_METHOD(...)						\
	template <size_t D>							\
	const auto &get() const & {						\
		static constexpr size_t counter_base = __COUNTER__;		\
		if constexpr (false) {}						\
		MAP(GEN_SCAFFOLD_FIELD_GET, /* NA */, __VA_ARGS__)		\
		else {								\
			static_error("bad scaffold get "_ss + $ss_type(This));	\
		}								\
	}									\
	template <size_t D>							\
	auto &get() & {								\
		static constexpr size_t counter_base = __COUNTER__;		\
		if constexpr (false) {}						\
		MAP(GEN_SCAFFOLD_FIELD_GET, /* NA */, __VA_ARGS__)		\
		else {								\
			static_error("bad scaffold get "_ss + $ss_type(This));	\
		}								\
	}

#define DEFINE_SCAFFOLD_OFFSET_METHOD(...)					\
	template <size_t I>							\
	static consteval size_t offset() {					\
		static constexpr size_t counter_base = __COUNTER__;		\
		if constexpr (false) {}						\
		MAP(GEN_SCAFFOLD_FIELD_OFFSET, /* NA */, __VA_ARGS__)		\
		else {								\
			static_error("out of bounds scaffold offset of "_ss	\
				+ $ss_type(This));				\
			return 0;						\
		}								\
	}

#define DEFINE_SCAFFOLD(...)							\
	template <size_t Align, bool AlignTopLevel, typename ... Ts>		\
	requires (sizeof...(Ts) == field_count)					\
	struct alignas(AlignTopLevel ? Align : 0) scaffold {			\
		static constexpr size_t counter_base = __COUNTER__;		\
		MAP(GEN_SCAFFOLD_FIELDS, This, __VA_ARGS__)			\
		DEFINE_SCAFFOLD_GET_METHOD(__VA_ARGS__)				\
		DEFINE_SCAFFOLD_OFFSET_METHOD(__VA_ARGS__)			\
	};

#if defined(__GNUC__) && !defined(__clang__)
#define GCC diagnostic pop
#endif

} // namespace rcgp
