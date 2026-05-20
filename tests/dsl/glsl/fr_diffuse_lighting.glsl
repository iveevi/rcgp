#version 460

#extension GL_EXT_scalar_block_layout : require

struct fwd_PointLight {
    vec3 position;
    vec3 color;
    vec3 velocity;
    float intensity;
};

layout (location = 0) smooth in vec3 lin0;
layout (location = 1) smooth in vec3 lin1;
layout (location = 2) smooth in vec2 lin2;

layout (location = 0) out vec3 lout0;

layout (set = 0, binding = 0) uniform sampler2D r0b0;

layout (set = 0, binding = 1) uniform sampler2D r0b1;

layout (set = 0, binding = 2) uniform sampler2D r0b2;

layout (std430, set = 0, binding = 0) readonly buffer Buffer0x0 {
    int count;
    fwd_PointLight lights[];
} r0b0;

void main()
{
#ifdef __clang__
    int lvar0;
    lvar0 = r0b0.count;
#elif defined(__GNUC__)
    vec2 lvar0;
    lvar0 = vec2(lin2);
#endif
    vec3 lvar1;
#ifdef __clang__
    lvar1 = vec3(lin0);
#elif defined(__GNUC__)
    lvar1 = vec3(lin1);
#endif
    vec3 lvar2;
#ifdef __clang__
    lvar2 = vec3(lin1);
    vec2 lvar3;
    lvar3 = vec2(lin2);
#elif defined(__GNUC__)
    lvar2 = vec3(lin0);
    int lvar3;
    lvar3 = r0b0.count;
#endif
    vec4 lvar4;
#ifdef __clang__
    lvar4 = texture(r0b0, lvar3);
#elif defined(__GNUC__)
    lvar4 = texture(r0b0, lvar0);
#endif
    vec3 lvar5;
    lvar5 = lvar4.xyz;
    float lvar6;
    lvar6 = 0;
    vec3 lvar7;
    lvar7 = vec3(lvar6, lvar6, lvar6);
    int lvar8;
    lvar8 = 0;
    while (true) {
        int lvar9;
#ifdef __clang__
        lvar9 = lvar8;
#elif defined(__GNUC__)
        lvar9 = lvar3;
#endif
        int lvar10;
#ifdef __clang__
        lvar10 = lvar0;
#elif defined(__GNUC__)
        lvar10 = lvar8;
#endif
        bool lvar11;
#ifdef __clang__
        lvar11 = (lvar9 < lvar10);
#elif defined(__GNUC__)
        lvar11 = (lvar10 < lvar9);
#endif
        bool lvar12;
        lvar12 = (!lvar11);
        bool lvar13;
        lvar13 = lvar12;
        bool lvar14;
        lvar14 = lvar13;
        if (lvar14) {
            break;
        }
        int lvar15;
        lvar15 = lvar8;
        vec3 lvar16;
#ifdef __clang__
        lvar16 = (r0b0.lights[lvar15].position - lvar1);
#elif defined(__GNUC__)
        lvar16 = (r0b0.lights[lvar15].position - lvar2);
#endif
        float lvar17;
#ifdef __clang__
        lvar17 = dot(lvar16, lvar16);
#elif defined(__GNUC__)
        lvar17 = 0.0001;
#endif
        float lvar18;
#ifdef __clang__
        lvar18 = 0.0001;
#elif defined(__GNUC__)
        lvar18 = dot(lvar16, lvar16);
#endif
        float lvar19;
#ifdef __clang__
        lvar19 = lvar17;
#elif defined(__GNUC__)
        lvar19 = lvar18;
#endif
        float lvar20;
#ifdef __clang__
        lvar20 = lvar18;
#elif defined(__GNUC__)
        lvar20 = lvar17;
#endif
        float lvar21;
        lvar21 = max(lvar19, lvar20);
        float lvar22;
        lvar22 = (r0b0.lights[lvar15].intensity / lvar21);
        vec3 lvar23;
        lvar23 = normalize(lvar16);
        float lvar24;
#ifdef __clang__
        lvar24 = dot(lvar2, lvar23);
#elif defined(__GNUC__)
        lvar24 = 0;
#endif
        float lvar25;
#ifdef __clang__
        lvar25 = 0;
#elif defined(__GNUC__)
        lvar25 = dot(lvar1, lvar23);
#endif
        float lvar26;
#ifdef __clang__
        lvar26 = lvar24;
#elif defined(__GNUC__)
        lvar26 = lvar25;
#endif
        float lvar27;
#ifdef __clang__
        lvar27 = lvar25;
#elif defined(__GNUC__)
        lvar27 = lvar24;
#endif
        float lvar28;
        lvar28 = max(lvar26, lvar27);
        float lvar29;
        lvar29 = lvar28;
        vec3 lvar30;
        lvar30 = (lvar5 * lvar29);
        vec3 lvar31;
        lvar31 = (lvar30 * r0b0.lights[lvar15].color);
        float lvar32;
        lvar32 = lvar22;
        vec3 lvar33;
        lvar33 = (lvar31 * lvar32);
        vec3 lvar34;
        lvar34 = (lvar7 + lvar33);
        lvar7 = lvar34;
        int lvar35;
        lvar35 = lvar8;
        int lvar36;
        lvar36 = 1;
        int lvar37;
        lvar37 = (lvar8 + lvar36);
        lvar8 = lvar37;
    }
    lout0 = lvar7;
}
