#pragma once

#include "intrinsics_vertex.hpp"
#include "intrinsics_compute.hpp"

#include "../dsl/array.hpp"
#include "../dsl/tracer.hpp"
#include "../util/cti.hpp"

namespace rcgp {

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

template <builtin T, template <builtin> typename Q = Smooth>
struct PerVertex : jems::handle {
	using element_type = T;
	static constexpr RateProperties properties = Q <T> ::rate;

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
	static constexpr RateProperties properties = Q <T> ::rate;

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

template <MeshPrimitive P, uint32_t MaxVertices, uint32_t MaxPrimitives, typename T = void>
struct MeshletPayload {
	static_assert(MaxVertices > 0, "MeshletPayload MaxVertices must be > 0");
	static_assert(MaxPrimitives > 0, "MeshletPayload MaxPrimitives must be > 0");

	mesh_vertex_positions <vec4> positions;
	mesh_primitive_triangles <uvec3> triangles;

	void allocate(u32 vertices, u32 primitives, $location) {
		jems::builtin_intrinsic(
			BuiltinIntrinsicCode::eSetMeshOutputsEXT,
			std::vector <Reference> { vertices, primitives },
			loc
		);
	}
};

template <MeshPrimitive P, uint32_t MaxVertices, uint32_t MaxPrimitives, user_defined T>
struct MeshletPayload <P, MaxVertices, MaxPrimitives, T> : T {
	static_assert(MaxVertices > 0, "MeshletPayload MaxVertices must be > 0");
	static_assert(MaxPrimitives > 0, "MeshletPayload MaxPrimitives must be > 0");

	mesh_vertex_positions <vec4> positions;
	mesh_primitive_triangles <uvec3> triangles;

	MeshletPayload() {
		constexpr_for(Is, T::field_count,
			([&] {
				using Field = T::fields::template get <Is>;
				using Unwrapped = Field::element_type;
				constexpr bool perprimitive = is_perprimitive_output_v <Field>;
				constexpr bool pervertex = is_pervertex_output_v <Field>;
				static_assert(perprimitive || pervertex, "MeshletPayload extra outputs must be PerVertex<> or PerPrimitive<>");

				constexpr auto count = perprimitive ? MaxPrimitives : MaxVertices;
				auto type = reconstruct_type <array <Unwrapped, count>> ();
				auto &counter = $tsb.mesh_output_counter;
				auto tout = StageOutput(type, counter++, Field::properties);
				$tsb.add_stage_output(tout);
				$tsb.mesh_perprimitive_outputs.emplace(tout.argi, perprimitive);

				this->template _rcgp_get <Is> ().override_reference(
					jems::stage_output(tout.type, tout.argi, tout.properties)
				);
			} (), ...)
		);
	}

	void allocate(u32 vertices, u32 primitives, $location) {
		jems::builtin_intrinsic(
			BuiltinIntrinsicCode::eSetMeshOutputsEXT,
			{ vertices, primitives }, loc
		);
	}
};

} // namespace rcgp
