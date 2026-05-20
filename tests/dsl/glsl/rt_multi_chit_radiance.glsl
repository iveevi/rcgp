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
    vec2 lvar0;
    lvar0 = vec2(hit_attribute);
    vec3 lvar1;
#ifdef __clang__
    lvar1 = gl_WorldRayOriginEXT;
    vec3 lvar2;
    lvar2 = gl_WorldRayDirectionEXT;
    float lvar3;
    lvar3 = gl_HitTEXT;
#elif defined(__GNUC__)
    lvar1 = gl_WorldRayDirectionEXT;
    float lvar2;
    lvar2 = gl_HitTEXT;
    vec3 lvar3;
    lvar3 = (lvar1 * lvar2);
#endif
    vec3 lvar4;
#ifdef __clang__
    lvar4 = (lvar2 * lvar3);
#elif defined(__GNUC__)
    lvar4 = gl_WorldRayOriginEXT;
#endif
    vec3 lvar5;
#ifdef __clang__
    lvar5 = (lvar1 + lvar4);
#elif defined(__GNUC__)
    lvar5 = (lvar4 + lvar3);
#endif
    float lvar6;
    lvar6 = 0;
    payload1 = lvar6;
#ifdef __clang__
    vec3 lvar7;
    lvar7 = vec3(lvar5);
#elif defined(__GNUC__)
    uint lvar7;
    lvar7 = 255;
#endif
    float lvar8;
#ifdef __clang__
    lvar8 = 0;
#elif defined(__GNUC__)
    lvar8 = 100;
#endif
    float lvar9;
#ifdef __clang__
    lvar9 = 1;
    float lvar10;
    lvar10 = 0;
    vec3 lvar11;
    lvar11 = vec3(lvar8, lvar9, lvar10);
#elif defined(__GNUC__)
    lvar9 = 0.001;
    vec3 lvar10;
    lvar10 = vec3(lvar5);
    float lvar11;
    lvar11 = 0;
#endif
    float lvar12;
#ifdef __clang__
    lvar12 = 0.001;
#elif defined(__GNUC__)
    lvar12 = 1;
#endif
    float lvar13;
#ifdef __clang__
    lvar13 = 100;
    uint lvar14;
    lvar14 = 255;
#elif defined(__GNUC__)
    lvar13 = 0;
    vec3 lvar14;
    lvar14 = vec3(lvar13, lvar12, lvar11);
#endif
    int lvar15;
    lvar15 = 13;
#ifdef __clang__
    traceRayEXT(r0b0, lvar15, lvar14, 1, 3, 1, lvar7, lvar12, lvar11, lvar13, 1);
#elif defined(__GNUC__)
    traceRayEXT(r0b0, lvar15, lvar7, 1, 3, 1, lvar10, lvar9, lvar14, lvar8, 1);
#endif
    vec3 lvar16;
    lvar16 = gl_WorldRayOriginEXT;
    vec3 lvar17;
    lvar17 = (lvar16 - lvar5);
    vec3 lvar18;
    lvar18 = normalize(lvar17);
    float lvar19;
    lvar19 = 0;
    float lvar20;
    lvar20 = 1;
    float lvar21;
    lvar21 = 0;
    vec3 lvar22;
#ifdef __clang__
    lvar22 = vec3(lvar19, lvar20, lvar21);
#elif defined(__GNUC__)
    lvar22 = vec3(lvar21, lvar20, lvar19);
#endif
    float lvar23;
#ifdef __clang__
    lvar23 = 2;
#elif defined(__GNUC__)
    lvar23 = dot(lvar18, lvar22);
#endif
    float lvar24;
#ifdef __clang__
    lvar24 = lvar23;
    vec3 lvar25;
    lvar25 = (lvar22 * lvar24);
    float lvar26;
    lvar26 = dot(lvar18, lvar22);
#elif defined(__GNUC__)
    lvar24 = 2;
    float lvar25;
    lvar25 = lvar24;
    vec3 lvar26;
    lvar26 = (lvar22 * lvar25);
#endif
    float lvar27;
#ifdef __clang__
    lvar27 = lvar26;
#elif defined(__GNUC__)
    lvar27 = lvar23;
#endif
    vec3 lvar28;
#ifdef __clang__
    lvar28 = (lvar25 * lvar27);
#elif defined(__GNUC__)
    lvar28 = (lvar26 * lvar27);
#endif
    vec3 lvar29;
    lvar29 = (lvar18 - lvar28);
    float lvar30;
    lvar30 = 0;
    vec3 lvar31;
    lvar31 = vec3(lvar30, lvar30, lvar30);
    payload2 = lvar31;
#ifdef __clang__
    vec3 lvar32;
    lvar32 = vec3(lvar5);
    vec3 lvar33;
    lvar33 = vec3(lvar29);
#elif defined(__GNUC__)
    uint lvar32;
    lvar32 = 255;
    float lvar33;
    lvar33 = 100;
#endif
    float lvar34;
    lvar34 = 0.001;
#ifdef __clang__
    float lvar35;
    lvar35 = 100;
    uint lvar36;
    lvar36 = 255;
#elif defined(__GNUC__)
    vec3 lvar35;
    lvar35 = vec3(lvar5);
    vec3 lvar36;
    lvar36 = vec3(lvar29);
#endif
    int lvar37;
    lvar37 = 0;
#ifdef __clang__
    traceRayEXT(r0b0, lvar37, lvar36, 2, 3, 2, lvar32, lvar34, lvar33, lvar35, 2);
#elif defined(__GNUC__)
    traceRayEXT(r0b0, lvar37, lvar32, 2, 3, 2, lvar35, lvar34, lvar36, lvar33, 2);
#endif
    float lvar38;
#ifdef __clang__
    lvar38 = payload1;
    vec3 lvar39;
    lvar39 = vec3(lvar38, lvar38, lvar38);
    float lvar40;
    lvar40 = 0.3;
#elif defined(__GNUC__)
    lvar38 = 0.3;
    float lvar39;
    lvar39 = lvar38;
    vec3 lvar40;
    lvar40 = (payload2 * lvar39);
#endif
    float lvar41;
#ifdef __clang__
    lvar41 = lvar40;
#elif defined(__GNUC__)
    lvar41 = payload1;
#endif
    vec3 lvar42;
#ifdef __clang__
    lvar42 = (payload2 * lvar41);
#elif defined(__GNUC__)
    lvar42 = vec3(lvar41, lvar41, lvar41);
#endif
    vec3 lvar43;
#ifdef __clang__
    lvar43 = (lvar39 + lvar42);
#elif defined(__GNUC__)
    lvar43 = (lvar42 + lvar40);
#endif
    payload0 = lvar43;
}
