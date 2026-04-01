#pragma once

#include "../dsl/aliases.hpp"
#include "../dsl/projection.hpp"
#include "resources.hpp"

namespace rcgp {

template <SystemValue G, ShaderStage S, builtin T>
struct read_only_intrinsic {
	// NOTE: for aggregates, we may need to inject...
	operator T() const {
		return T::reinterpret(jems::system_value(G));
	}
};

template <SystemValue G, ShaderStage S, builtin T>
struct write_only_intrinsic {
	auto &operator=(const T &value) {
		auto self = jems::system_value(G);
		jems::store(self, value);
		return value;
	}
};

template <SystemValue G, ShaderStage S, typename T>
struct projection <read_only_intrinsic <G, S, T>> {
	using type = T;
};

} // namespace rcgp
