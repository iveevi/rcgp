#pragma once

#include <cstddef>
#include <cstdint>

#include "../dsl/array.hpp"
#include "../dsl/jems.hpp"
#include "../dsl/matrix.hpp"
#include "../dsl/scalar.hpp"
#include "../dsl/vector.hpp"
#include "../util/cti.hpp"
#include "concepts.hpp"
#include "static_string.hpp"

namespace rcgp {

// TODO: use normal functions...
template <typename T>
struct reconstructor_t {
	static jems::handle main($location) {
		static_error("type reconstructor for "_ss + $ss_type(T) + " has not been implemented yet"_ss);
	}
};

template <typename T>
struct reconstructor_t <scalar <T>> {
	static jems::handle main($location) {
		return jems::type_loc(loc, primitive_of <T> ());
	}
};

template <typename T, size_t N>
struct reconstructor_t <vector <T, N>> {
	static jems::handle main($location) {
		return jems::type_loc(loc, primitive_of <T, N> ());
	}
};

template <typename T, size_t N, size_t M>
struct reconstructor_t <matrix <T, N, M>> {
	static jems::handle main($location) {
		return jems::type_loc(loc, primitive_of <T, N, M> ());
	}
};

template <typename T, int64_t N>
struct reconstructor_t <array <T, N>> {
	static jems::handle main($location) {
		auto base = reconstructor_t <T> ::main(loc);
		return jems::type_loc(loc, Array(base, N));
	}
};

template <>
struct reconstructor_t <std::nullptr_t> {
	static jems::handle main($location) {
		return jems::handle();
	}
};

template <aggregate T>
struct reconstructor_t <T> {
	static jems::handle main($location) {
		Struct st;
		
		st.name = std::string($ss_type(T).view());

		auto ftn = [&] <size_t I> {
			using F = T::fields::template get <I>;
			st.emplace_back(reconstructor_t <F> ::main(loc));
			st.fields.emplace_back(T::_field_names[I]);
		};

		constexpr_for(Is, T::field_count,
			(ftn.template operator() <Is> (), ...);
		);

		return jems::type_loc(loc, st);
	}
};

template <typename T>
jems::handle reconstruct_type($location)
{
	return reconstructor_t <T> ::main(loc);
}

} // namespace rcgp
