#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require

layout (set = 0, binding = 0) uniform accelerationStructureEXT r0b0;

layout (location = 0) rayPayloadEXT vec3 payload0;

layout (set = 0, binding = 0) uniform writeonly image2D r0b0;

void main()
{
    payload0 = vec3(0, 0, 0);
    traceRayEXT(r0b0, 1, 255, 0, 3, 0, vec3(0, 0, 0), 0.001, vec3(0, 0, 1), 100, 0);
    imageStore(r0b0, ivec2(uvec2(gl_LaunchIDEXT.xy)), vec4(payload0, 0));
}
