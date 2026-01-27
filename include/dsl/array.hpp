#pragma once

#include "projection.hpp"

namespace rcgp {

template <typename T, int64_t N = -1>
struct array : public jems::handle {
	template <projectively_int_scalar U>
	T operator[](const U &idx) const {
		T result;
		auto access = jems::array_access(_ref, project(idx));
		inject_reference(result, access);
		return result;
	}
};

} // namespace rcgp
