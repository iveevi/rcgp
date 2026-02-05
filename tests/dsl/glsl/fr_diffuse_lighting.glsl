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
    vec2 lvar0;
    vec3 lvar1;
    vec3 lvar2;
    int lvar3;
    texture(r0b0, lin2);
    vec4 lvar4;
    lvar4 = texture(r0b0, lin2);
    vec3 lvar5;
    lvar5 = lvar4.xyz;
    float lvar6;
    lvar6 = 0;
    vec3 lvar7;
    lvar7 = vec3(lvar6, lvar6, lvar6);
    int lvar8;
    lvar8 = 0;
    while (true) {
        bool lvar9;
        lvar9 = (lvar8 < r0b0.count);
        if ((!lvar9)) {
            break;
        }
        vec3 lvar10;
        vec3 lvar11;
        vec3 lvar12;
        float lvar13;
        vec3 lvar14;
        lvar14 = (r0b0.lights[lvar8].position - lin0);
        float lvar15;
        lvar15 = 0.0001;
        dot(lvar14, lvar14);
        float lvar16;
        lvar16 = dot(lvar14, lvar14);
        max(lvar16, lvar15);
        float lvar17;
        lvar17 = max(lvar16, lvar15);
        normalize(lvar14);
        vec3 lvar18;
        lvar18 = normalize(lvar14);
        float lvar19;
        lvar19 = 0;
        dot(lin1, lvar18);
        float lvar20;
        lvar20 = dot(lin1, lvar18);
        max(lvar20, lvar19);
        float lvar21;
        lvar21 = max(lvar20, lvar19);
        vec3 lvar22;
        lvar22 = (lvar5 * lvar21);
        vec3 lvar23;
        lvar23 = (lvar22 * r0b0.lights[lvar8].color);
        vec3 lvar24;
        lvar24 = (lvar23 * (r0b0.lights[lvar8].intensity / lvar17));
        vec3 lvar25;
        lvar25 = (lvar7 + lvar24);
        lvar7 = lvar25;
        int lvar26;
        lvar26 = 1;
        lvar8 = (lvar8 + lvar26);
    }
    lout0 = lvar7;
}
