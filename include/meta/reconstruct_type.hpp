#pragma once

#include "../dsl/jems.hpp"
#include "expand_reflection.hpp"
#include "reflection.hpp"
#include "field_access.hpp"
#include "static_string.hpp"
#include "../util/cti.hpp"

// TODO: get rid of the mains!
// TODO: use type cache to avoid redefining things...
// also need a DCE pass...
template <typename T>
struct reconstructor_t {
	static jems::handle main($location) {
		static_error("type reconstructor for "_ss + $ss_type(T) + " has not been implemented yet"_ss);
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

template <>
struct reconstructor_t <std::nullptr_t> {
	static jems::handle main($location) {
		return jems::handle();
	}
};

template <typename Original, typename ... Args>
struct reconstructor_t <aggregate_reflection <Original, Args...>> {
	static void collect(AggregateType &aggregate, jems::handle handle) {
		if (handle)
			aggregate.emplace_back(handle);
	}

	static jems::handle main($location) {
		AggregateType aggregate;
		(collect(aggregate, reconstructor_t <Args> ::main(loc)), ...);
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
	// TODO: use a more direct generator...
	using R = expand_reflection <T> ::type;
	return reconstructor_t <R> ::main(loc);
}
