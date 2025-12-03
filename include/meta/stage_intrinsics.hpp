#pragma once

#include "../dsl/primitives.hpp"
#include "reconstruct_type.hpp"

// TODO: also the associated stage...
template <GlobalIntrinsic G, reflected T>
struct read_only_intrinsic {
	// NOTE: for aggregates, we may need to inject...
	operator T() const {
		return T::reinterpret(jems::global_intrinsic(G));
	}

	void override_reference(const Reference &ref) {
		fatal("bad override");
	}
};

template <GlobalIntrinsic G, reflected T>
struct projection <read_only_intrinsic <G, T>> {
	using type = T;
};

using InstanceIndex = read_only_intrinsic <GlobalIntrinsic::eInstanceIndex, i32>;
using VertexIndex = read_only_intrinsic <GlobalIntrinsic::eVertexIndex, i32>;

// Required result of the vertex shader
struct Position {
	Position() = default;

	Position(const vec4 &value, $location) {
		jems::store_loc(loc,
			jems::global_intrinsic_loc(loc, GlobalIntrinsic::eScreenPosition),
			value
		);
	}

	void override_reference(const Reference &ref) {
		fatal("bad override");
	}
};

enum Topology {
	Point,
	Line,
	Triangle,
	Patch,
};

// Optional result of the vertex shader
template <primitive T, RateProperties P>
struct Interpolant : jems::handle {
	Interpolant() = default;

	template <typename U>
	requires std::is_convertible_v <U, T>
	Interpolant(const U &value, $location)
		: handle(jems::thread_output_loc(loc,
			reconstruct_type <T> (), 0, P
		))
	{
		jems::store_loc(loc, *this, T(value));
	}
};

template <primitive T>
using Smooth = Interpolant <T, RateProperties::eSmooth>;

template <primitive T>
using Flat = Interpolant <T, RateProperties::eFlat>;

template <primitive T>
using NoPerspective = Interpolant <T, RateProperties::eNoPerspective>;

// Explicit wrapper to indicate that fragment shaders
// inputs are from the previous subpass
template <typename T>
struct SubpassInput : T {};

// // Required result of the fragment shader
// template <typename T>
// struct SubpassResult : public T {
// 	SubpassResult(const T &value) : T(value) {}
// };
//
// // Required result of the task shader
// // 
// // for multiple groups per task shader invocation
// // users can either have a struct with multiple instances
// // or use an array of packets; in either case the
// // payload must be the same and the total number of
// // packets issues must be finite
// template <typename T>
// struct MeshGroupPacket : ivec3 {};
//
// enum PatchType {
// 	eTriangle,
// };
//
// // Required result of the mesh shader
// template <PatchType Patch, size_t MaxVertices, size_t MaxPrimitives>
// struct PatchResult {
// 	i32 vertices;
// 	i32 primitives;
//
// 	// void set_output(i32 vertices, i32 primitives) {}
// };
