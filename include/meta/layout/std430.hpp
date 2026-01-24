#pragma once

#include "pod.hpp"

#include "../../dsl/matrix.hpp"
#include "../../dsl/scalar.hpp"
#include "../../dsl/vector.hpp"
#include "../../util/cti.hpp"
#include "../reflection.hpp"
#include "../scaffold.hpp"
#include "../static_string.hpp"

namespace std430_layout {

template <typename T>
struct layout_engine {
	static_error("s::layout_engine not implemented for type "_ss + $ss_type(T));
};

template <typename Original, typename ... Ts>
struct layout_engine <aggregate_reflection <Original, Ts...>> {
	static constexpr bool dynamic = (is_dynamic_reflection_v <Ts> || ...);
	static constexpr size_t alignment = [] {
		if constexpr (dynamic)
			return 0;
		else
			return std::max({ layout_engine <Ts> ::alignment... });
	} ();

	using hint = scaffold_hint <
		Tlist <typename layout_engine <Ts> ::hint...>,
		alignment
	>;
};

template <typename T, int64_t N>
requires (N > 0)
struct layout_engine <array_reflection <T, N>> {
	static constexpr size_t alignment = layout_engine <T> ::alignment;
	using hint = scaffold_hint <
		std::array <typename layout_engine <T> ::hint, N>,
		alignment
	>;
};

template <typename T>
struct layout_engine <array_reflection <T, -1>> {
	static constexpr size_t alignment = layout_engine <T> ::alignment;
	using hint = scaffold_hint <
		unsized_array <typename layout_engine <T> ::hint>,
		alignment
	>;
};

template <native_scalar T, size_t N, size_t M>
struct layout_engine <primitive_reflection <matrix <T, N, M>>> {
	// TODO: equals the alignment of row vector
	static constexpr size_t alignment = 16;
	using hint = scaffold_hint <
		pod::mat <M, N, T>,
		alignment
	>;
};

template <native_scalar T, size_t D>
struct layout_engine <primitive_reflection <vector <T, D>>> {
	static constexpr size_t alignment = [] constexpr {
		switch (D) {
		case 4:
		case 3:
			return 16;
		case 2:
			return 8;
		}
	} ();

	using hint = scaffold_hint <
		pod::vec <D, T>,
		alignment
	>;
};

template <native_scalar T>
struct layout_engine <primitive_reflection <scalar <T>>> {
	static constexpr size_t alignment = alignof(T);
	using hint = scaffold_hint <T, alignment>;
};

} // namespace std430_layout
