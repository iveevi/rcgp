#pragma once

#include <concepts>
#include <type_traits>
#include <variant>

#include "../util/logging.hpp"
#include "../util/cti.hpp"
#include "../dsl/array.hpp"
#include "../dsl/aliases.hpp"
#include "../dsl/projection.hpp"
#include "reconstruct_type.hpp"
#include "reflection.hpp"
#include "inject_reference.hpp"
#include "reflection_builder.hpp"

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
using LocalInvocationID = read_only_intrinsic <GlobalIntrinsic::eLocalInvocationID, uvec3>;
using WorkGroupID = read_only_intrinsic <GlobalIntrinsic::eWorkGroupID, uvec3>;
using GlobalInvocationID = read_only_intrinsic <GlobalIntrinsic::eGlobalInvocationID, uvec3>;

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
	void emit_mesh_tasks(u32 x, u32 y = 1, u32 z = 1, $location) const
	{
		jems::builtin_intrinsic_loc(
			loc,
			BuiltinIntrinsicCode::eEmitMeshTasksEXT,
			x, y, z
		);
	}
};

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

// Required result of the task shader
template <reflected T>
struct TaskPayload : T {
	using reflection = typename T::reflection;
	DEFINE_REFLECTION_STAMP();

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

	using reflection = typename T::reflection;
	DEFINE_REFLECTION_STAMP();

	MeshletPayload()
	{
		if (Tracer::singleton.records.empty())
			return;

		using R = typename T::reflection;
		static_assert(is_aggregate_reflection_v <R>,
			"MeshletPayload extra outputs must be an aggregate");

		constexpr_for(Is, R::field_count,
			([&] {
				using Field = R::template field_type <Is>;
				using Unwrapped = unwrap_output_type_t <Field>;
				constexpr bool perprimitive = is_perprimitive_output_v <Field>;
				constexpr bool pervertex = is_pervertex_output_v <Field>;
				static_assert(perprimitive || pervertex,
					"MeshletPayload extra outputs must be PerVertex<> or PerPrimitive<>");

				constexpr auto count = perprimitive ? MaxPrimitives : MaxVertices;
				auto type = reconstruct_type <array <Unwrapped, count>> ();
				auto &counter = $tsb.context.mesh_output_counter;
				auto tout = ThreadOutput(type, counter++, Field::properties);
				$tsb.context.add_thread_output(tout);
				$tsb.context.mesh_perprimitive_outputs.emplace(tout.argi, perprimitive);

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
