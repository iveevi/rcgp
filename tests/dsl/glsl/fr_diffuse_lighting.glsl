#version 460

struct fwdxPointLight {
    vec3 position;
    vec3 color;
    vec3 velocity;
    float intensity;
};

layout (location = 0) smooth in vec3 lin0;
layout (location = 1) smooth in vec3 lin1;
layout (location = 2) smooth in vec2 lin2;

layout (location = 0) out vec3 lout0;

layout (std430, set = 0, binding = 0) readonly buffer Buffer0x0 {
    int count;
    fwdxPointLight lights[];
} r0b0;

layout (set = 0, binding = 0) uniform sampler2D r0b0;

void main()
{
    texture(r0b0, lin2);
    vec4 lvar0;
    lvar0 = texture(r0b0, lin2);
    vec3 lvar1;
    lvar1 = lvar0.xyz;
    float lvar2;
    lvar2 = 0;
    vec3 lvar3;
    lvar3 = vec3(lvar2, lvar2, lvar2);
    int lvar4;
    lvar4 = 0;
    while (true) {
        bool lvar5;
        lvar5 = (lvar4 < r0b0.count);
        bool lvar6;
        lvar6 = (!lvar5);
        if (lvar6) {
            break;
        }
        vec3 lvar7;
        lvar7 = (r0b0.lights[lvar4].position - lin0);
        float lvar8;
        lvar8 = 0.0001;
        dot(lvar7, lvar7);
        float lvar9;
        lvar9 = dot(lvar7, lvar7);
        max(lvar9, lvar8);
        float lvar10;
        lvar10 = max(lvar9, lvar8);
        float lvar11;
        lvar11 = (r0b0.lights[lvar4].intensity / lvar10);
        normalize(lvar7);
        vec3 lvar12;
        lvar12 = normalize(lvar7);
        float lvar13;
        lvar13 = 0;
        dot(lin1, lvar12);
        float lvar14;
        lvar14 = dot(lin1, lvar12);
        max(lvar14, lvar13);
        float lvar15;
        lvar15 = max(lvar14, lvar13);
        vec3 lvar16;
        lvar16 = (lvar1 * lvar15);
        vec3 lvar17;
        lvar17 = (lvar16 * r0b0.lights[lvar4].color);
        vec3 lvar18;
        lvar18 = (lvar17 * lvar11);
        vec3 lvar19;
        lvar19 = (lvar3 + lvar18);
        lvar3 = lvar19;
        int lvar20;
        lvar20 = 1;
        int lvar21;
        lvar21 = (lvar4 + lvar20);
        lvar4 = lvar21;
    }
    lout0 = lvar3;
}
