#pragma once

#include <rcgp.hpp>

using namespace rcgp;

struct Ray {
	float3 origin;
	float3 direction;

	$reflection(origin, direction);
};

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

