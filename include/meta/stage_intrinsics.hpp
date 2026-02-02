#pragma once

#include <type_traits>

#include "../util/cti.hpp"
#include "../dsl/array.hpp"
#include "../dsl/aliases.hpp"
#include "../dsl/projection.hpp"
#include "reconstruct_type.hpp"
#include "inject_reference.hpp"

namespace rcgp {

// TODO: move intrinsics definition to a dedicated header
// TODO: should also mark the stage so that we check at shader module definion...
template <GlobalIntrinsic G, ShaderStage S, primitive T>
struct read_only_intrinsic {
	// NOTE: for aggregates, we may need to inject...
	operator T() const {
		return T::reinterpret(jems::global_intrinsic(G));
	}
};

template <GlobalIntrinsic G, ShaderStage S, primitive T>
struct write_only_intrinsic {
	auto &operator=(const T &value) {
		auto self = jems::global_intrinsic(G);
		jems::store(self, value);
		return value;
	}
};

template <GlobalIntrinsic G, ShaderStage S, typename T>
struct projection <read_only_intrinsic <G, S, T>> {
	using type = T;
};

using InstanceIndex = read_only_intrinsic <GlobalIntrinsic::eInstanceIndex, ShaderStage::eVertex, i32>;
using VertexIndex = read_only_intrinsic <GlobalIntrinsic::eVertexIndex, ShaderStage::eVertex, i32>;
using ClipPosition = write_only_intrinsic <GlobalIntrinsic::eClipPosition, ShaderStage::eVertex, vec4>;

// TODO: should also support mesh, task...
using LocalInvocationID = read_only_intrinsic <GlobalIntrinsic::eLocalInvocationID, ShaderStage::eCompute, uvec3>;
using WorkGroupID = read_only_intrinsic <GlobalIntrinsic::eWorkGroupID, ShaderStage::eCompute, uvec3>;
using GlobalInvocationID = read_only_intrinsic <GlobalIntrinsic::eGlobalInvocationID, ShaderStage::eCompute, uvec3>;

template <uint32_t X, uint32_t Y = 1, uint32_t Z = 1>
struct WorkGroup {
	static constexpr uint32_t size_x = X;
	static constexpr uint32_t size_y = Y;
	static constexpr uint32_t size_z = Z;
	LocalInvocationID local_index;
	WorkGroupID workgroup_index;
	GlobalInvocationID global_index;
};

template <uint32_t X, uint32_t Y = 1, uint32_t Z = 1>
struct TaskGroup : WorkGroup <X, Y, Z> {
	void dispatch_mesh_groups(u32 x, u32 y = 1, u32 z = 1, $location) const
	{
		jems::builtin_intrinsic_loc(
			loc,
			BuiltinIntrinsicCode::eEmitMeshTasksEXT,
			x, y, z
		);
	}
};

// Interpolation qualifiers for varying attributes
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

// Smooth interpolation
template <primitive T>
struct Smooth : Interpolant <T, RateProperties::eSmooth> {
	using Interpolant <T, RateProperties::eSmooth> ::Interpolant;
};

template <primitive T>
Smooth(const T &) -> Smooth <T>;

// Flat (no) interpolation
template <primitive T>
struct Flat : Interpolant <T, RateProperties::eFlat> {
	using Interpolant <T, RateProperties::eFlat> ::Interpolant;
};

template <primitive T>
Flat(const T &) -> Flat <T>;

// Smooth (without perspective correction) interpolation
template <primitive T>
struct NoPerspective : Interpolant <T, RateProperties::eNoPerspective> {
	using Interpolant <T, RateProperties::eNoPerspective> ::Interpolant;
};

template <primitive T>
NoPerspective(const T &) -> NoPerspective <T>;

// Required result of the task shader
template <typename T>
struct TaskPayload : T {
	TaskPayload() {
		inject_reference(static_cast <T &> (*this),
			jems::global_intrinsic(GlobalIntrinsic::eTaskPayload)
		);
	}
};

template <typename T>
struct mesh_vertex_positions : jems::handle {
	mesh_vertex_positions() : handle(jems::global_intrinsic(GlobalIntrinsic::eMeshVertices)) {}

	template <projectively_int_scalar U>
	T operator[](const U &idx) const {
		T result;
		auto access = jems::array_access(_ref, project(idx));
		inject_reference(result, access);
		return result;
	}
};

template <typename T>
struct mesh_primitive_triangles : jems::handle {
	mesh_primitive_triangles()
		: handle(jems::global_intrinsic(GlobalIntrinsic::ePrimitiveTriangleIndices)) {}

	template <projectively_int_scalar U>
	T operator[](const U &idx) const {
		T result;
		auto access = jems::array_access(_ref, project(idx));
		inject_reference(result, access);
		return result;
	}
};

template <template <primitive> typename Q>
struct rate_properties_for;

template <>
struct rate_properties_for <Smooth> {
	static constexpr RateProperties value = RateProperties::eSmooth;
};

template <>
struct rate_properties_for <Flat> {
	static constexpr RateProperties value = RateProperties::eFlat;
};

template <>
struct rate_properties_for <NoPerspective> {
	static constexpr RateProperties value = RateProperties::eNoPerspective;
};

template <primitive T, template <primitive> typename Q = Smooth>
struct PerVertex : jems::handle {
	using element_type = T;
	static constexpr RateProperties properties = rate_properties_for <Q> ::value;

	template <projectively_int_scalar U>
	T operator[](const U &idx) const {
		T result;
		auto access = jems::array_access(_ref, project(idx));
		inject_reference(result, access);
		return result;
	}
};

template <primitive T, template <primitive> typename Q = Smooth>
struct PerPrimitive : jems::handle {
	using element_type = T;
	static constexpr RateProperties properties = rate_properties_for <Q> ::value;

	template <projectively_int_scalar U>
	T operator[](const U &idx) const {
		T result;
		auto access = jems::array_access(_ref, project(idx));
		inject_reference(result, access);
		return result;
	}
};

TYPE_TRAIT(is_perprimitive_output);
	template <primitive T, template <primitive> typename Q>
	TYPE_TRAIT_INCLUDES(is_perprimitive_output, PerPrimitive <T, Q>);

TYPE_TRAIT(is_pervertex_output);
	template <primitive T, template <primitive> typename Q>
	TYPE_TRAIT_INCLUDES(is_pervertex_output, PerVertex <T, Q>);

template <typename T>
struct unwrap_output_type {
	using type = T;
};

template <primitive T, template <primitive> typename Q>
struct unwrap_output_type <PerVertex <T, Q>> {
	using type = T;
};

template <primitive T, template <primitive> typename Q>
struct unwrap_output_type <PerPrimitive <T, Q>> {
	using type = T;
};

template <typename T>
using unwrap_output_type_t = typename unwrap_output_type <T> ::type;

template <MeshPrimitive P, uint32_t MaxVertices, uint32_t MaxPrimitives, typename T = void>
struct MeshletPayload;

template <MeshPrimitive P, uint32_t MaxVertices, uint32_t MaxPrimitives>
struct MeshletPayload <P, MaxVertices, MaxPrimitives, void> {
	static_assert(MaxVertices > 0, "MeshletPayload MaxVertices must be > 0");
	static_assert(MaxPrimitives > 0, "MeshletPayload MaxPrimitives must be > 0");

	mesh_vertex_positions <vec4> positions;
	mesh_primitive_triangles <uvec3> triangles;

	void allocate(u32 vertices, u32 primitives, $location)
	{
		jems::builtin_intrinsic_loc(
			loc,
			BuiltinIntrinsicCode::eSetMeshOutputsEXT,
			vertices,
			primitives
		);
	}
};

template <MeshPrimitive P, uint32_t MaxVertices, uint32_t MaxPrimitives, typename T>
struct MeshletPayload : T {
	static_assert(MaxVertices > 0, "MeshletPayload MaxVertices must be > 0");
	static_assert(MaxPrimitives > 0, "MeshletPayload MaxPrimitives must be > 0");

	mesh_vertex_positions <vec4> positions;
	mesh_primitive_triangles <uvec3> triangles;

	MeshletPayload()
	{
		if (Tracer::singleton.records.empty())
			return;

		static_assert(aggregate <T>,
			"MeshletPayload extra outputs must be an aggregate");

		constexpr_for(Is, T::field_count,
			([&] {
				using Field = T::fields::template get <Is>;
				using Unwrapped = unwrap_output_type_t <Field>;
				constexpr bool perprimitive = is_perprimitive_output_v <Field>;
				constexpr bool pervertex = is_pervertex_output_v <Field>;
				static_assert(perprimitive || pervertex,
					"MeshletPayload extra outputs must be PerVertex<> or PerPrimitive<>");

				constexpr auto count = perprimitive ? MaxPrimitives : MaxVertices;
				auto type = reconstruct_type <array <Unwrapped, count>> ();
				auto &counter = $tsb.mesh_output_counter;
				auto tout = ThreadOutput(type, counter++, Field::properties);
				$tsb.add_thread_output(tout);
				$tsb.mesh_perprimitive_outputs.emplace(tout.argi, perprimitive);

				auto &field = this->template _rcgp_get <Is> ();
				inject_reference(field, jems::thread_output(tout));
			}(), ...)
		);
	}

	void allocate(u32 vertices, u32 primitives, $location)
	{
		jems::builtin_intrinsic_loc(
			loc,
			BuiltinIntrinsicCode::eSetMeshOutputsEXT,
			vertices,
			primitives
		);
	}
};

} // namespace rcgp
