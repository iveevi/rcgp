#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../util/variant.hpp"

namespace rcgp {

// TODO: one single instruction_nodes.hpp header...
struct Block;
struct Instruction;

using SharedBlockReference = std::shared_ptr <Block>;
using Reference = std::shared_ptr <Instruction>;

template <typename T, size_t N>
struct VectorType {};

template <typename T, size_t N, size_t M>
struct MatrixType {};

struct PrimitiveType : variant <
	bool,
	int32_t,
	uint32_t,
	float,
	
	// Vector types
	VectorType <uint32_t, 2>,
	VectorType <uint32_t, 3>,
	VectorType <uint32_t, 4>,

	VectorType <int32_t, 2>,
	VectorType <int32_t, 3>,
	VectorType <int32_t, 4>,

	VectorType <float, 2>,
	VectorType <float, 3>,
	VectorType <float, 4>,

	// Matrix types
	MatrixType <int32_t, 2, 2>,
	MatrixType <int32_t, 3, 3>,
	MatrixType <int32_t, 4, 4>,
	MatrixType <uint32_t, 2, 2>,
	MatrixType <uint32_t, 3, 3>,
	MatrixType <uint32_t, 4, 4>,
	MatrixType <float, 2, 2>,
	MatrixType <float, 3, 3>,
	MatrixType <float, 4, 4>
> {
	using variant_self::variant;
};

struct AggregateType : std::vector <Reference> {
	std::string name;
};

struct ArrayType {
	Reference base;
	int64_t size;
};

struct Type : variant <
	PrimitiveType,
	AggregateType,
	ArrayType
> {
	using variant_self::variant;

	std::string repr() const;
};

} // namespace rcgp
