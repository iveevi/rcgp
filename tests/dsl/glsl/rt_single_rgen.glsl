#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require

layout (set = 0, binding = 0) uniform accelerationStructureEXT r0b0;

layout (location = 0) rayPayloadEXT vec3 payload0;

layout (set = 0, binding = 0) uniform writeonly image2D r0b0;

void main()
{
    uvec3 lvar0;
    lvar0 = gl_LaunchIDEXT;
    float lvar1;
    lvar1 = 0;
    vec3 lvar2;
    lvar2 = vec3(lvar1, lvar1, lvar1);
    payload0 = lvar2;
#ifdef __clang__
    float lvar3;
    lvar3 = 0;
    vec3 lvar4;
    lvar4 = vec3(lvar3, lvar3, lvar3);
#elif defined(__GNUC__)
    uint lvar3;
    lvar3 = 255;
    float lvar4;
    lvar4 = 100;
#endif
    float lvar5;
#ifdef __clang__
    lvar5 = 0;
#elif defined(__GNUC__)
    lvar5 = 0.001;
#endif
    float lvar6;
    lvar6 = 0;
#ifdef __clang__
    float lvar7;
    lvar7 = 1;
    vec3 lvar8;
    lvar8 = vec3(lvar5, lvar6, lvar7);
#elif defined(__GNUC__)
    vec3 lvar7;
    lvar7 = vec3(lvar6, lvar6, lvar6);
    float lvar8;
    lvar8 = 1;
#endif
    float lvar9;
#ifdef __clang__
    lvar9 = 0.001;
#elif defined(__GNUC__)
    lvar9 = 0;
#endif
    float lvar10;
#ifdef __clang__
    lvar10 = 100;
    uint lvar11;
    lvar11 = 255;
#elif defined(__GNUC__)
    lvar10 = 0;
    vec3 lvar11;
    lvar11 = vec3(lvar10, lvar9, lvar8);
#endif
    int lvar12;
    lvar12 = 1;
#ifdef __clang__
    traceRayEXT(r0b0, lvar12, lvar11, 0, 1, 0, lvar4, lvar9, lvar8, lvar10, 0);
    uvec2 lvar13;
    lvar13 = lvar0.xy;
    uvec2 lvar14;
    lvar14 = uvec2(lvar13);
    ivec2 lvar15;
    lvar15 = ivec2(lvar14);
    float lvar16;
    lvar16 = 0;
    vec4 lvar17;
    lvar17 = vec4(payload0, lvar16);
    imageStore(r0b0, lvar15, lvar17);
#elif defined(__GNUC__)
    traceRayEXT(r0b0, lvar12, lvar3, 0, 1, 0, lvar7, lvar5, lvar11, lvar4, 0);
    float lvar13;
    lvar13 = 0;
    vec4 lvar14;
    lvar14 = vec4(payload0, lvar13);
    uvec2 lvar15;
    lvar15 = lvar0.xy;
    uvec2 lvar16;
    lvar16 = uvec2(lvar15);
    ivec2 lvar17;
    lvar17 = ivec2(lvar16);
    imageStore(r0b0, lvar17, lvar14);
#endif
}
