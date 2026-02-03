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

static PushConstant <View> view;

static AttributeStream <float3> position;
