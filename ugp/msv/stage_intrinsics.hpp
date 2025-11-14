#pragma once

#include "../dsl/primitives.hpp"
#include "../msv/reconstruct_type.hpp"

// Required result of the vertex shader
struct Position {
	Position(const vec4 &value, $location) {
		jems::store_loc(loc,
			jems::intrinsic_loc(loc, GlobalIntrinsic::eSVPosition),
			value
		);
	}
};

enum Topology {
	Point,
	Line,
	Triangle,
	Patch,
};

// Optional result of the vertex shader
template <typename T, ThreadOutput::Properties P>
struct Interpolant : jems::handle {
	// TODO: require reflection_expander, like reference
	template <typename U>
	requires std::is_convertible_v <U, T>
	Interpolant(const U &value, $location)
		: handle(jems::intrinsic_loc(loc,
			ThreadOutput(reconstruct_type <T> (), 0, P)
		))
	{
		jems::store_loc(loc, *this, T(value));
	}
};

template <typename T>
using Smooth = Interpolant <T, ThreadOutput::eSmooth>;

template <typename T>
using Flat = Interpolant <T, ThreadOutput::eFlat>;

template <typename T>
using NoPerspective = Interpolant <T, ThreadOutput::eNoPerspective>;

// Required result of the fragment shader
template <typename T>
struct SubpassResult : public T {
	SubpassResult(const T &value) : T(value) {}
};

// Required result of the task shader
// 
// for multiple groups per task shader invocation
// users can either have a struct with multiple instances
// or use an array of packets; in either case the
// payload must be the same and the total number of
// packets issues must be finite
template <typename T>
struct MeshGroupPacket : ivec3 {};

enum PatchType {
	eTriangle,
};

// Required result of the mesh shader
template <PatchType Patch, size_t MaxVertices, size_t MaxPrimitives>
struct PatchResult {
	i32 vertices;
	i32 primitives;

	// void set_output(i32 vertices, i32 primitives) {}
};
