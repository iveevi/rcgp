#include <rcgp.hpp>

using namespace rcgp;

struct Camera {
	float4x4 view_proj;

	$reflection(view_proj);
};

struct Light {
	float3 position;
	f32 intensity;

	$reflection(position, intensity);
};

struct MaterialGroup {
	UniformBuffer <Camera> camera;
	Sampler2D albedo;
	RStorageBuffer <array <Light>> lights;

	$reflection(camera, albedo, lights);
};

static UniformBuffer <Camera> camera;
static Sampler2D albedo;
static RStorageBuffer <array <Light>> lights;
static ResourceGroup <MaterialGroup> material_group;
static PushConstant <Camera> camera_pc;

using CameraUboHandle = ResourceType <UniformBuffer <Camera>>;
static_assert(std::same_as <
	CameraUboHandle,
	MirrorBuffer <Camera, layouts::std430, vk::BufferUsageFlagBits::eUniformBuffer>
>);

using LightsSsboHandle = ResourceType <RStorageBuffer <array <Light>>>;
static_assert(std::same_as <
	LightsSsboHandle,
	MirrorBuffer <array <Light>, layouts::std430, vk::BufferUsageFlagBits::eStorageBuffer>
>);

using AlbedoHandle = ResourceType <Sampler2D>;
static_assert(std::same_as <AlbedoHandle, MirrorSampler>);

using CameraPcHandle = ResourceType <PushConstant <Camera>>;
static_assert(std::same_as <CameraPcHandle, layouts::apply_t <Camera, layouts::std430>>);

using GroupHandle = ResourceType <ResourceGroup <MaterialGroup>>;
static_assert(std::same_as <decltype(GroupHandle::camera), CameraUboHandle>);
static_assert(std::same_as <decltype(GroupHandle::albedo), MirrorSampler>);
static_assert(std::same_as <decltype(GroupHandle::lights), LightsSsboHandle>);

using CameraUboFor = ResourceTypeFor <camera>;
static_assert(std::same_as <CameraUboFor, CameraUboHandle>);

using GroupHandleFor = ResourceTypeFor <material_group>;
static_assert(std::same_as <GroupHandleFor, GroupHandle>);

using CameraPcFor = ResourceTypeFor <camera_pc>;
static_assert(std::same_as <CameraPcFor, CameraPcHandle>);
