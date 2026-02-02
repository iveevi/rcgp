#pragma once

#include <cstdint>
#include <optional>
#include <print>

#include "instruction_enums.hpp"
#include "instruction_types.hpp"

namespace rcgp {

struct GlobalResource {
	Reference type;
	GlobalResourceKind kind;
	GlobalResourceLayout layout;
	GlobalResourceAccess access = GlobalResourceAccess::eReadWrite;

	std::optional <uint32_t> group;
	std::optional <uint32_t> index;
	std::optional <uint32_t> offset;

	std::string repr() const {
		return std::format("GlobalResource({}, {})",
			rcgp::repr(kind), rcgp::repr(layout));
	}
};

// TODO: separate one for subroutine arguments...

struct ThreadInput {
	Reference type;
	// corresponds to actual order
	uint32_t argi;

	std::string repr() const {
		return "ThreadInput";
	}
};

struct ThreadOutput {
	Reference type;
	uint32_t argi;

	RateProperties properties;

	std::string repr() const {
		return "ThreadOutput";
	}
};

} // namespace rcgp
