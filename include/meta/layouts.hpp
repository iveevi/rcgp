#pragma once

#include <algorithm>
#include <type_traits>

#include "../dsl/array.hpp"
#include "../dsl/matrix.hpp"
#include "../dsl/scalar.hpp"
#include "../dsl/vector.hpp"
#include "../util/cti.hpp"
#include "pod.hpp"
#include "scaffold.hpp"
#include "static_string.hpp"

namespace rcgp::layouts {

namespace detail {

struct std430_policy {
	template <typename T>
	static consteval size_t unsized_array_alignment(size_t element_alignment) {
		return element_alignment;
	}

	template <native_scalar T, size_t N, size_t M>
	static consteval size_t matrix_alignment() {
		return 16;
	}

	template <native_scalar T, size_t D>
	static consteval size_t vector_alignment() {
		switch (D) {
		case 4:
		case 3:
			return 16;
		case 2:
			return 8;
		}
	}
};

struct scalar_policy {
	template <typename T>
	static consteval size_t unsized_array_alignment(size_t) {
		return 0;
	}

	template <native_scalar T, size_t N, size_t M>
	static consteval size_t matrix_alignment() {
		return alignof(T);
	}

	template <native_scalar T, size_t D>
	static consteval size_t vector_alignment() {
		return alignof(T);
	}
};

template <typename Policy, typename T>
struct layout_rules {
	static_error("layout_rules not implemented for type "_ss + $ss_type(T));
};

// TODO: is this necessary?
template <typename T>
struct is_dynamic_layout : std::false_type {};

template <typename T>
struct is_dynamic_layout <array <T, -1>> : std::true_type {};

template <typename List>
struct is_dynamic_fields;

template <typename ... Ts>
struct is_dynamic_fields <Tlist <Ts...>>
	: std::bool_constant <(is_dynamic_layout <Ts> ::value || ...)> {};

template <aggregate T>
struct is_dynamic_layout <T> : is_dynamic_fields <typename T::fields> {};

template <typename T>
constexpr bool is_dynamic_layout_v = is_dynamic_layout <T> ::value;

template <typename Policy, typename List>
struct layout_rules_list;

template <typename Policy, typename ... Ts>
struct layout_rules_list <Policy, Tlist <Ts...>> {
	static constexpr bool dynamic = (is_dynamic_layout_v <Ts> || ...);
	static constexpr size_t alignment = [] {
		if constexpr (dynamic)
			return 0;
		else
			return std::max({ layout_rules <Policy, Ts> ::alignment... });
	} ();

	using hint = scaffold_hint <
		Tlist <typename layout_rules <Policy, Ts> ::hint...>,
		alignment
	>;
};

template <typename Policy, aggregate T>
struct layout_rules <Policy, T> : layout_rules_list <Policy, typename T::fields> {};

template <typename Policy, typename T, int64_t N>
requires (N > 0)
struct layout_rules <Policy, array <T, N>> {
	static constexpr size_t alignment = layout_rules <Policy, T> ::alignment;
	using hint = scaffold_hint <
		std::array <typename layout_rules <Policy, T> ::hint, N>,
		alignment
	>;
};

template <typename Policy, typename T>
struct layout_rules <Policy, array <T, -1>> {
	static constexpr size_t alignment = Policy::template unsized_array_alignment <T>(
		layout_rules <Policy, T> ::alignment
	);
	using hint = scaffold_hint <
		unsized_array <typename layout_rules <Policy, T> ::hint>,
		alignment
	>;
};

template <typename Policy, native_scalar T, size_t N, size_t M>
struct layout_rules <Policy, matrix <T, N, M>> {
	static constexpr size_t alignment = Policy::template matrix_alignment <T, N, M>();
	using hint = scaffold_hint <
		pod::mat <M, N, T>,
		alignment
	>;
};

template <typename Policy, native_scalar T, size_t D>
struct layout_rules <Policy, vector <T, D>> {
	static constexpr size_t alignment = Policy::template vector_alignment <T, D>();
	using hint = scaffold_hint <
		pod::vec <D, T>,
		alignment
	>;
};

template <typename Policy, native_scalar T>
struct layout_rules <Policy, scalar <T>> {
	static constexpr size_t alignment = alignof(T);
	using hint = scaffold_hint <T, alignment>;
};

} // namespace detail

template <typename T>
using std430 = detail::layout_rules <detail::std430_policy, T>;

template <typename T>
using scalar = detail::layout_rules <detail::scalar_policy, T>;

} // namespace rcpg::layouts
