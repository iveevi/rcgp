#pragma once

#include <type_traits>

#include "../util/tlist.hpp"
#include "macro_hell.hpp"
#include "scaffold.hpp"
#include "this_injection.hpp"

// Helper aliases
template <typename P, typename ... Ts>
using snipped_tlist = rcgp::Tlist <Ts...>;

// Building the reflection for aggregates
#define GEN_AGGREGATE_FIELD(T, field)	, decltype(field)

#define DEFINE_FIELD_TYPE_LIST(...)							\
	using fields = snipped_tlist							\
		<void MAP(GEN_AGGREGATE_FIELD, /* NA */ , __VA_ARGS__)>;		\
	static constexpr size_t field_count = fields::size;

// Generating a tuple-like get method
#define GEN_AGGREGATE_FIELD_REFERENCE(T, field)	\
	else if constexpr (D == (__COUNTER__ - counter_base - 1)) { return field; }

#define DEFINE_FIELD_REFERENCE(R, ...)							\
	template <size_t D>								\
	auto &_rcgp_get() R {								\
		static constexpr size_t counter_base = __COUNTER__;			\
		if constexpr (false) {}							\
		MAP(GEN_AGGREGATE_FIELD_REFERENCE, /* NA */, __VA_ARGS__)		\
		else									\
			static_error("invalid field access of "_ss + $ss_type(This));	\
	}

// Generating the override reference method
#define GEN_OVERRIDE_FIELD_REFERENCE(T, field)	\
	field.override_reference(jems::field_access(ref, __COUNTER__ - counter_base - 1));

#define DEFINE_OVERRIDE_REFERENCE(...)						\
	void override_reference(const Reference &ref) {				\
		static constexpr size_t counter_base = __COUNTER__;		\
		MAP(GEN_OVERRIDE_FIELD_REFERENCE, /* ... */, __VA_ARGS__)	\
	}

// Field names
#define GEN_FIELD_NAME(T, field) #field,

#define DEFINE_FIELD_NAMES(...) 						\
	static constexpr auto _field_names = std::array {			\
		MAP(GEN_FIELD_NAME, /* ... */, __VA_ARGS__)			\
	};

// Full reflection information for aggregates
#define $reflection(...)					\
	DEFINE_THIS();						\
	using _rcgp_aggregate = std::type_identity <This>;	\
								\
	DEFINE_FIELD_TYPE_LIST(__VA_ARGS__);			\
	DEFINE_SCAFFOLD(__VA_ARGS__);				\
								\
	DEFINE_FIELD_REFERENCE(const, __VA_ARGS__);		\
	DEFINE_FIELD_REFERENCE(/*&*/, __VA_ARGS__);		\
								\
	DEFINE_FIELD_NAMES(__VA_ARGS__);			\
								\
	DEFINE_OVERRIDE_REFERENCE(__VA_ARGS__);
