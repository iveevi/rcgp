#pragma once

#include <cstdint>
#include <optional>

#include "instruction_enums.hpp"
#include "instruction_types.hpp"

struct GlobalResource {
	Reference type;
	using Kind = GlobalResourceKind;
	using Layout = GlobalResourceLayout;
	Kind kind;
	Layout layout;

	// group := descriptor set index
	std::optional <uint32_t> group;
	// index := descriptor binding
	std::optional <uint32_t> index;
	// push constants use this to disambiguate multiple blocks
	std::optional <uint32_t> push_constant_index;
	// push constants use this to determine offset within the shared block
	std::optional <uint32_t> push_constant_offset;

	std::string repr() const {
		return "GlobalResource";
	}
};

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
