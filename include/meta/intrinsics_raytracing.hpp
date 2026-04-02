#pragma once

#include "intrinsics_common.hpp"
#include "contract.hpp"
#include "reconstruct_type.hpp"
#include "reflection.hpp"

#include "../dsl/tracer.hpp"

namespace rcgp {

// Raytracing intrinsics
struct RaytracingAccelerationStructure : jems::handle {
	using handle_type = RaytracingAccelerationStructure;
};

// Populate ray flags to exactly match the EXT specification:
// https://github.com/KhronosGroup/GLSL/blob/main/extensions/ext/GLSL_EXT_ray_tracing.txt
enum RayFlags {
	eNone = 0U,
	eOpaque = 1U,
	eNoOpaque = 2U,
	eTerminateOnFirstHit = 4U,
	eSkipClosestHitShader = 8U,
	eCullBackFacingTriangles = 16U,
	eCullFrontFacingTriangles = 32U,
	eCullOpaque = 64U,
	eCullNoOpaque = 128U,
};

constexpr RayFlags operator|(RayFlags a, RayFlags b)
{
	return RayFlags(std::to_underlying(a) | std::to_underlying(b));
}

template <traced T, RayFlags F = eNone>
struct TraceGroup {
	static constexpr auto flags = F;

	using value_type = T;
};

struct Ray {
	float3 origin;
	float3 direction;

	$reflection(origin, direction);
};

template <auto &ref>
struct Dispatcher {
	using R = reference_base_of <ref>;
	using T = R::value_type;
	static constexpr void *address = (void *)&ref;

	struct handle_type : jems::handle {
		handle_type &operator=(const T &value) {
			jems::store(*this, value);
			return *this;
		}

		operator T() const {
			T value;
			value.override_reference(*this);
			return value;
		}

		void trace(
			RaytracingAccelerationStructure as,
			Ray ray,
			f32 tmin,
			f32 tmax,
			u32 mask = 0xff,
			$location
		) {
			auto h = jems::builtin_intrinsic(
				BuiltinIntrinsicCode::eTraceRaysEXT,
				{
					as, i32(std::to_underlying(R::flags)), mask,
					jems::constant(int32_t(0), loc),
					jems::constant(int32_t(1), loc),
					jems::constant(int32_t(-1), loc),  // miss index (unresolved)
					ray.origin, tmin,
					ray.direction, tmax,
					jems::constant(int32_t(-1), loc)   // payload index (unresolved)
				}, loc
			);
			$tsb.add_trace_group((void *)&ref, Reference(h));
		}
	};
};

template <auto &ref>
struct Receiver {
	using R = reference_base_of <ref>;
	using T = R::value_type;
	static constexpr void *address = (void *)&ref;

	struct handle_type : jems::handle {
		handle_type &operator=(const T &value) {
			jems::store(*this, value);
			return *this;
		}
	};
};

template <auto &ref>
inline auto dispatcher = Dispatcher <ref> ();

TYPE_TRAIT(is_ray_dispatcher);
	template <auto &ref>
	TYPE_TRAIT_INCLUDES(is_ray_dispatcher, Dispatcher <ref>);

template <auto &ref>
inline auto receiver = Receiver <ref> ();

TYPE_TRAIT(is_ray_receiver);
	template <auto &ref>
	TYPE_TRAIT_INCLUDES(is_ray_receiver, Receiver <ref>);

using HitTime = read_only_intrinsic <SystemValue::eHitT, ShaderStage::eClosestHit, f32>;
using LaunchID = read_only_intrinsic <SystemValue::eLaunchID, ShaderStage::eRayGeneration, uvec3>;
using LaunchSize = read_only_intrinsic <SystemValue::eLaunchSize, ShaderStage::eRayGeneration, uvec3>;
using PrimitiveID = read_only_intrinsic <SystemValue::ePrimitiveID, ShaderStage::eClosestHit, u32>;
using WorldRayDirection = read_only_intrinsic <SystemValue::eWorldRayDirection, ShaderStage::eClosestHit, vec3>;
using WorldRayOrigin = read_only_intrinsic <SystemValue::eWorldRayOrigin, ShaderStage::eClosestHit, vec3>;
using InstanceCustomIndex = read_only_intrinsic <SystemValue::eInstanceCustomIndex, ShaderStage::eClosestHit, u32>;

template <>
TYPE_TRAIT_INCLUDES(is_global_resource, RaytracingAccelerationStructure);

inline jems::handle resource_intrinsic(const RaytracingAccelerationStructure &, uint32_t binding)
{
	return jems::global_resource(GlobalResource {
		nullptr,
		GlobalResourceKind::eAccelerationStructure,
		GlobalResourceLayout::eNone,
		GlobalResourceAccess::eRead,
		std::nullopt,
		binding
	});
}

template <auto &ref>
TYPE_TRAIT_INCLUDES(is_global_resource, Dispatcher <ref>);

template <auto &ref>
jems::handle resource_intrinsic(const Dispatcher <ref> &, uint32_t _)
{
	using R = reference_base_of <ref>;
	using T = R::value_type;

	return jems::global_resource(GlobalResource {
		reconstruct_type <T> (),
		GlobalResourceKind::eRayDispatcherPayload,
		GlobalResourceLayout::eNone,
		GlobalResourceAccess::eRead,
		std::nullopt,
		std::nullopt
	});
}

template <auto &ref>
TYPE_TRAIT_INCLUDES(is_global_resource, Receiver <ref>);

template <auto &ref>
jems::handle resource_intrinsic(const Receiver <ref> &, uint32_t _)
{
	using R = reference_base_of <ref>;
	using T = R::value_type;

	return jems::global_resource(GlobalResource {
		reconstruct_type <T> (),
		GlobalResourceKind::eRayReceiverPayload,
		GlobalResourceLayout::eNone,
		GlobalResourceAccess::eRead,
		std::nullopt,
		std::nullopt
	});
}

} // namespace rcgp
