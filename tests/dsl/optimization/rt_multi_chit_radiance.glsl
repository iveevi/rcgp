#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require

hitAttributeEXT vec2 hit_attribute;

layout (set = 0, binding = 0) uniform accelerationStructureEXT r0b0;

layout (location = 1) rayPayloadEXT float payload1;

layout (location = 2) rayPayloadEXT vec3 payload2;

layout (location = 0) rayPayloadInEXT vec3 payload0;

void main()
{
    vec3 lvar0 = (gl_WorldRayOriginEXT + (gl_WorldRayDirectionEXT * gl_HitTEXT));
    payload1 = 0;
    traceRayEXT(r0b0, 13, 255, 1, 3, 1, vec3(lvar0), 0.001, vec3(0, 1, 0), 100, 1);
    vec3 lvar1 = normalize((gl_WorldRayOriginEXT - lvar0));
    vec3 lvar2 = vec3(0, 1, 0);
    payload2 = vec3(0, 0, 0);
    traceRayEXT(r0b0, 0, 255, 2, 3, 2, vec3(lvar0), 0.001, vec3((lvar1 - ((lvar2 * 2) * dot(lvar1, lvar2)))), 100, 2);
    payload0 = (vec3(payload1, payload1, payload1) + (payload2 * 0.3));
}
