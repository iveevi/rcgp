#pragma once

#include <type_traits>

#include "../util/cti.hpp"
#include "../dsl/array.hpp"
#include "../dsl/aliases.hpp"
#include "../dsl/projection.hpp"
#include "reconstruct_type.hpp"

namespace rcgp {

// TODO: move intrinsics definition to a dedicated header
// TODO: should also mark the stage so that we check at shader module definion...
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

using InstanceIndex = read_only_intrinsic <SystemValue::eInstanceIndex, ShaderStage::eVertex, i32>;
using VertexIndex = read_only_intrinsic <SystemValue::eVertexIndex, ShaderStage::eVertex, i32>;
using ClipPosition = write_only_intrinsic <SystemValue::eClipPosition, ShaderStage::eVertex, vec4>;

// TODO: should also support mesh, task...
using LocalInvocationID = read_only_intrinsic <SystemValue::eLocalInvocationID, ShaderStage::eCompute, uvec3>;
using WorkGroupID = read_only_intrinsic <SystemValue::eWorkGroupID, ShaderStage::eCompute, uvec3>;
using GlobalInvocationID = read_only_intrinsic <SystemValue::eGlobalInvocationID, ShaderStage::eCompute, uvec3>;

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
		jems::builtin_intrinsic(
			BuiltinIntrinsicCode::eEmitMeshTasksEXT,
			std::vector <Reference> { x, y, z },
			loc
		);
	}
};

// Interpolation qualifiers for varying attributes
template <builtin T, RateProperties P>
struct Interpolant : jems::handle {
	Interpolant() = default;

	template <typename U>
	requires std::is_convertible_v <U, T>
	Interpolant(const U &value, $location)
		: handle(jems::stage_output(
			reconstruct_type <T> (), 0, P, loc
		))
	{
		jems::store(*this, T(value), loc);
	}
};

// Smooth interpolation
template <builtin T>
struct Smooth : Interpolant <T, RateProperties::eSmooth> {
	using Interpolant <T, RateProperties::eSmooth> ::Interpolant;
};

template <builtin T>
Smooth(const T &) -> Smooth <T>;

// Flat (no) interpolation
template <builtin T>
struct Flat : Interpolant <T, RateProperties::eFlat> {
	using Interpolant <T, RateProperties::eFlat> ::Interpolant;
};

template <builtin T>
Flat(const T &) -> Flat <T>;

// Smooth (without perspective correction) interpolation
template <builtin T>
struct NoPerspective : Interpolant <T, RateProperties::eNoPerspective> {
	using Interpolant <T, RateProperties::eNoPerspective> ::Interpolant;
};

template <builtin T>
NoPerspective(const T &) -> NoPerspective <T>;

// Required result of the task shader
template <traced T>
struct TaskPayload : T {
	TaskPayload() {
		this->override_reference(jems::system_value(SystemValue::eTaskPayload));
	}
};

template <traced T>
struct mesh_vertex_positions : jems::handle {
	mesh_vertex_positions() : handle(jems::system_value(SystemValue::eMeshVertices)) {}

	template <projectively_int_scalar U>
	T operator[](const U &idx) const {
		T result;
		auto access = jems::array_access(_ref, project(idx));
		result.override_reference(access);
		return result;
	}
};

template <typename T>
struct mesh_primitive_triangles : jems::handle {
	mesh_primitive_triangles()
		: handle(jems::system_value(SystemValue::ePrimitiveTriangleIndices)) {}

	template <projectively_int_scalar U>
	T operator[](const U &idx) const {
		T result;
		auto access = jems::array_access(_ref, project(idx));
		result.override_reference(access);
		return result;
	}
};

template <template <builtin> typename Q>
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

template <builtin T, template <builtin> typename Q = Smooth>
struct PerVertex : jems::handle {
	using element_type = T;
	static constexpr RateProperties properties = rate_properties_for <Q> ::value;

	template <projectively_int_scalar U>
	T operator[](const U &idx) const {
		T result;
		auto access = jems::array_access(_ref, project(idx));
		result.override_reference(access);
		return result;
	}
};

template <builtin T, template <builtin> typename Q = Smooth>
struct PerPrimitive : jems::handle {
	using element_type = T;
	static constexpr RateProperties properties = rate_properties_for <Q> ::value;

	template <projectively_int_scalar U>
	T operator[](const U &idx) const {
		T result;
		auto access = jems::array_access(_ref, project(idx));
		result.override_reference(access);
		return result;
	}
};

TYPE_TRAIT(is_perprimitive_output);
	template <builtin T, template <builtin> typename Q>
	TYPE_TRAIT_INCLUDES(is_perprimitive_output, PerPrimitive <T, Q>);

TYPE_TRAIT(is_pervertex_output);
	template <builtin  T, template <builtin> typename Q>
	TYPE_TRAIT_INCLUDES(is_pervertex_output, PerVertex <T, Q>);

template <typename T>
struct unwrap_output_type {
	using type = T;
};

template <builtin T, template <builtin> typename Q>
struct unwrap_output_type <PerVertex <T, Q>> {
	using type = T;
};

template <builtin T, template <builtin> typename Q>
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
		jems::builtin_intrinsic(
			BuiltinIntrinsicCode::eSetMeshOutputsEXT,
			std::vector <Reference> { vertices, primitives },
			loc
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

		static_assert(user_defined <T>,
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
				auto tout = StageOutput(type, counter++, Field::properties);
				$tsb.add_stage_output(tout);
				$tsb.mesh_perprimitive_outputs.emplace(tout.argi, perprimitive);

				this->template _rcgp_get <Is> ().override_reference(
					jems::stage_output(tout.type, tout.argi, tout.properties)
				);
			}(), ...)
		);
	}

	void allocate(u32 vertices, u32 primitives, $location)
	{
		jems::builtin_intrinsic(
			BuiltinIntrinsicCode::eSetMeshOutputsEXT,
			std::vector <Reference> { vertices, primitives },
			loc
		);
	}
};

} // namespace rcgp
