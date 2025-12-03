#pragma once

#include "reflection.hpp"
#include <cstddef>

// Forward declarations
template <reflected T, int64_t N>
struct array;

template <typename T>
using unsized_array = std::vector <T>;

// Pair of type and manual alignment
template <typename T, size_t N>
struct scaffold_hint {
	using type = T;
	
	static constexpr size_t value = N;
};

// Scaffold for primitive/built-in types
template <size_t Align, typename T>
struct alignas(Align) scaffold_fundamental {
	T value;

	constexpr scaffold_fundamental() = default;

	template <typename U>
	requires (std::is_trivially_constructible_v <T, U>)
	constexpr scaffold_fundamental(const U &value_) : value(value_) {}
};

// Scaffold for structual types with fields
template <size_t Align, typename T>
struct alignas(Align) scaffold_structural : T {
	using T::T;

	constexpr scaffold_structural() = default;
	constexpr scaffold_structural(const T &rhs) : T(rhs) {}
	constexpr scaffold_structural &operator=(const T &rhs) {
		T::operator=(rhs);
		return *this;
	}
};

// Scaffold lookup procedure
template <typename Hint, typename View>
struct scaffold_lookup {
	static_assert(false,
	       ($ss("no scaffold_lookup registered for hint { ")
		+ $ss_type(Hint) + $ss(" } and view { ")
		+ $ss_type(View) + $ss(" }")).view());
};

// Structural types
template <typename Mapped, size_t Align, typename View>
struct scaffold_lookup <scaffold_hint <Mapped, Align>, View> {
	using type = scaffold_structural <Align, Mapped>;
};

// Primitive/built-in types
template <typename Mapped, size_t Align, typename View>
requires (std::is_fundamental_v <Mapped>)
struct scaffold_lookup <scaffold_hint <Mapped, Align>, View> {
	using type = scaffold_fundamental <Align, Mapped>;
};

// Aggregate types
template <typename ... Ts, size_t Align, aggregate View>
struct scaffold_lookup <scaffold_hint <sequence <Ts...>, Align>, View> {
	using type = View::template scaffold <Align, Ts...>;
};

// Statically sized array types
template <typename Element, size_t N1, size_t Align, reflected ElementView, int64_t N2>
struct scaffold_lookup <
	scaffold_hint <std::array <Element, N1>, Align>,
	array <ElementView, N2>
>
{
	static_assert(N1 == N2, "bad");
	// NOTE: assuming that the array's alignment (Align) is negligible
	// since it should be equal to the element-wise alignment; might not
	// be true for completely scalar block layout
	using element = scaffold_lookup <Element, ElementView> ::type;
	using type = std::array <element, N1>;
};

// Dynamically sized array types
template <typename Element, size_t Align, reflected ElementView>
struct scaffold_lookup <
	scaffold_hint <unsized_array <Element>, Align>,
	array <ElementView, -1>
>
{
	using element = scaffold_lookup <Element, ElementView> ::type;
	using type = unsized_array <element>;
};

// Generating the scaffold in the reflection building process
#define GEN_SCAFFOLD_FIELDS(T, field)						\
	using hint_##field = Ts...[__COUNTER__ - counter_base - 1];		\
	scaffold_lookup <hint_##field, decltype(T::field)> ::type field;

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
			static_assert(false,					\
			($ss("out of bounds scaffold access of ")		\
				 + $ss_type(This)).view());			\
		}								\
	}

#define DEFINE_SCAFFOLD_OFFSET_METHOD(...)					\
	template <size_t I>							\
	static consteval size_t offset() {					\
		static constexpr size_t counter_base = __COUNTER__;		\
		if constexpr (false) {}						\
		MAP(GEN_SCAFFOLD_FIELD_OFFSET, /* NA */, __VA_ARGS__)		\
		else {								\
			static_assert(false,					\
			($ss("out of bounds scaffold offset of ")		\
				 + $ss_type(This)).view());			\
			return 0;						\
		}								\
	}

#define DEFINE_SCAFFOLD(...)							\
	template <size_t Align, typename ... Ts>				\
	requires (sizeof...(Ts) == reflection::field_count)			\
	struct alignas(Align) scaffold {					\
		static constexpr size_t counter_base = __COUNTER__;		\
		MAP(GEN_SCAFFOLD_FIELDS, This, __VA_ARGS__)			\
		DEFINE_SCAFFOLD_GET_METHOD(__VA_ARGS__)				\
		DEFINE_SCAFFOLD_OFFSET_METHOD(__VA_ARGS__)			\
	};
