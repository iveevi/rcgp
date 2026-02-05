#pragma once

#include <rcgp.hpp>

using namespace rcgp;

struct Ray {
	float3 origin;
	float3 direction;

	$reflection(origin, direction);
};

namespace fwd {

struct View {
	float4x4 model;
	float4x4 view;
	float4x4 proj;

	$reflection(model, view, proj);
};

struct PointLight {
	vec3 position;
	vec3 color;
	vec3 velocity;
	f32 intensity;

	$reflection(position, color, velocity, intensity);
};

struct PointLightArray {
	i32 count;
	array <PointLight> lights;

	$reflection(count, lights);
};

struct Material {
	Sampler2D albedo;
	Sampler2D specular;
	Sampler2D roughness;

	$reflection(albedo, specular, roughness);
};

static PushConstant <View> view;

static AttributeStream <float3> position;
static AttributeStream <float3> normal;
static AttributeStream <float2> uv;

static RStorageBuffer <PointLightArray> lights;

static ResourceGroup <Material> material;

} // namespace fwd

namespace meshlets {

struct ViewData {
	u32 meshlet_count;
	u32 task_group_width;
	float4x4 view_proj;
	array <float4, 6> frustum_planes;

	$reflection(view_proj, frustum_planes, meshlet_count, task_group_width);
};

struct Slice {
	u32 offset;
	u32 size;
};

struct MeshletData {
	// TODO: use Slice
	u32 vertex_offset;
	u32 vertex_count;
	u32 primitive_offset;
	u32 primitive_count;
	vec4 bounds;

	$reflection(vertex_offset, vertex_count, primitive_offset, primitive_count, bounds);
};

// TODO: support TaskPayload <u32> with assignment...
struct TaskPayloadData {
	u32 meshlet;

	$reflection(meshlet);
};

struct MeshOutputs {
	PerVertex <vec3> color;

	$reflection(color);
};

static PushConstant <ViewData> view;

static RStorageBuffer <array <float4>> positions;
static RStorageBuffer <array <MeshletData>> meshlets;

static struct {
	// TODO: use this to test conditional addition of
	// the scalar extension
	RStorageBuffer <array <u32>> vertices;
	RStorageBuffer <array <uint3>, layouts::scalar> triangles;
	RStorageBuffer <array <float3>, layouts::scalar> colors;
} buffers;

} // namespace meshlets
