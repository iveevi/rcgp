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

template <typename T>
jems::handle reconstruct_type($location);

template <typename T>
auto reconstructor_for(std::type_identity <scalar <T>>, $location)
{
	return jems::type_loc(loc, primitive_of <T> ());
}

template <typename T, size_t N>
auto reconstructor_for(std::type_identity <vector <T, N>>, $location)
{
	return jems::type_loc(loc, primitive_of <T, N> ());
}

template <typename T, size_t N, size_t M>
auto reconstructor_for(std::type_identity <matrix <T, N, M>>, $location)
{
	return jems::type_loc(loc, primitive_of <T, N, M> ());
}

template <typename T, int64_t N>
auto reconstructor_for(std::type_identity <array <T, N>>, $location)
{
	auto base = reconstruct_type <T> (loc);
	return jems::type_loc(loc, Array(base, N));
}

inline auto reconstructor_for(std::type_identity <jems::null>, $location)
{
	return jems::handle();
}

template <user_defined T>
auto reconstructor_for(std::type_identity <T>, $location)
{
	Struct st;
	
	st.name = std::string($ss_type(T).view());

	auto ftn = [&] <size_t I> {
		using F = T::fields::template get <I>;
		st.emplace_back(reconstruct_type <F> (loc));
		st.fields.emplace_back(T::_field_names[I]);
	};

	constexpr_for(Is, T::field_count,
		(ftn.template operator() <Is> (), ...);
	);

	return jems::type_loc(loc, st);
}

template <typename T>
jems::handle reconstruct_type(const std::source_location &loc)
{
	return reconstructor_for(std::type_identity <T> (), loc);
}

} // namespace rcgp
