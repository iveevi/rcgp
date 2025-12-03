#pragma once

#include "../dsl/jems.hpp"
#include "../dsl/primitives.hpp"
#include "reflection.hpp"
#include "expand_reflection.hpp"
#include "static_string.hpp"
#include "parameter_block_filter.hpp"

template <typename T>
struct reconstructor_t {
	static jems::handle main($location) {
		static_assert(false, ($ss("type reconstructor for ") + $ss_type(T) + $ss(" has not been implemented yet")).view());
	}
};

template <typename T>
struct reconstructor_t <primitive_reflection <scalar <T>>> {
	static jems::handle main($location) {
		return jems::type_loc(loc, T());
	}
};

template <typename T, size_t N>
struct reconstructor_t <primitive_reflection <vector <T, N>>> {
	static jems::handle main($location) {
		return jems::type_loc(loc, VectorType <T, N> ());
	}
};

template <typename T, size_t N, size_t M>
struct reconstructor_t <primitive_reflection <matrix <T, N, M>>> {
	static jems::handle main($location) {
		return jems::type_loc(loc, MatrixType <T, N, M> ());
	}
};

template <typename T, int64_t N>
struct reconstructor_t <array_reflection <T, N>> {
	static jems::handle main($location) {
		auto base = reconstructor_t <T> ::main();
		return jems::type_loc(loc, ArrayType(base, N));
	}
};

template <typename Original, typename ... Args>
struct reconstructor_t <aggregate_reflection <Original, Args...>> {
	static bool collect(AggregateType &aggregate, jems::handle handle) {
		aggregate.push_back(Reference(handle));
		return true;
	}

	static jems::handle main($location) {
		AggregateType aggregate;
		std::apply([&](auto ... xs) {
			std::make_tuple(collect(
				aggregate,
				reconstructor_t <decltype(xs)> ::main(loc)
			)...);
		}, std::tuple <Args...> ());

		return jems::type_loc(loc, aggregate);
	}
};

template <typename T, size_t ... Is>
struct reconstructor_t <field_trace <T, Is...>> {
	static jems::handle main($location) {
		return reconstructor_t <T> ::main(loc);
	}
};

template <typename T>
jems::handle reconstruct_type($location)
{
	using R = expand_reflection <T> ::type;
	return reconstructor_t <R> ::main(loc);
}
