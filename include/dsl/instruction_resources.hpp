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

struct Argument {
	Reference type;
	uint32_t argi;

	std::string repr() const {
		return "Argument";
	}
};

struct StageInput {
	Reference type;
	uint32_t argi;

	std::string repr() const {
		return "ThreadInput";
	}
};

struct StageOutput {
	Reference type;
	uint32_t argi;

	RateProperties properties;

	std::string repr() const {
		return "ThreadOutput";
	}
};

} // namespace rcgp
