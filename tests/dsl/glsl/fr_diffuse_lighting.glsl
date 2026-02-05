#version 460

struct fwdxPointLight {
    vec3 position;
    vec3 color;
    vec3 velocity;
    float intensity;
};

layout (location = 0) in vec3 lin0;
layout (location = 1) in vec3 lin1;
layout (location = 2) in vec2 lin2;

layout (location = 0)  out vec3 lout0;

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
    int lvar4;
    texture(r0b0, lin2);
    vec4 lvar5;
    lvar5 = texture(r0b0, lin2);
    vec3 lvar6;
    lvar6 = lvar5.xyz;
    float lvar7;
    lvar7 = 0;
    vec3 lvar8;
    lvar8 = vec3(lvar7, lvar7, lvar7);
    int lvar9;
    lvar9 = 0;
    while (true) {
        bool lvar10;
        lvar10 = (lvar9 < r0b0.count);
        if ((!lvar10)) {
            break;
        }
        vec3 lvar11;
        vec3 lvar12;
        vec3 lvar13;
        float lvar14;
        vec3 lvar15;
        lvar15 = (r0b0.lights[lvar9].position - lin0);
        float lvar16;
        lvar16 = 0.0001;
        dot(lvar15, lvar15);
        float lvar17;
        lvar17 = dot(lvar15, lvar15);
        max(lvar17, lvar16);
        float lvar18;
        lvar18 = max(lvar17, lvar16);
        normalize(lvar15);
        vec3 lvar19;
        lvar19 = normalize(lvar15);
        float lvar20;
        lvar20 = 0;
        dot(lin1, lvar19);
        float lvar21;
        lvar21 = dot(lin1, lvar19);
        max(lvar21, lvar20);
        float lvar22;
        lvar22 = max(lvar21, lvar20);
        vec3 lvar23;
        lvar23 = (lvar6 * lvar22);
        vec3 lvar24;
        lvar24 = (lvar23 * r0b0.lights[lvar9].color);
        vec3 lvar25;
        lvar25 = (lvar24 * (r0b0.lights[lvar9].intensity / lvar18));
        vec3 lvar26;
        lvar26 = (lvar8 + lvar25);
        lvar8 = lvar26;
        int lvar27;
        lvar27 = 1;
        lvar9 = (lvar9 + lvar27);
    }
    lout0 = lvar8;
}
