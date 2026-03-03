#pragma once

#include "projection.hpp"

namespace rcgp {

template <typename T, int64_t N = -1>
struct array : public jems::handle {
	template <projectively_int_scalar U>
	T operator[](const U &idx) const {
		T result;
		auto access = jems::array_access(_ref, project(idx));
		result.override_reference(access);
		return result;
	}

	scalar <int32_t> length() const {
		if constexpr (N >= 0) {
			return scalar <int32_t> (N);
		} else {
			auto intr = jems::builtin_intrinsic(BuiltinIntrinsicCode::eUnsizedArrayLength, { _ref });
			return scalar <int32_t> ::reinterpret(intr);
		}
	}
};

} // namespace rcgp
