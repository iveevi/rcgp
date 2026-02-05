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
    float lvar5;
    lvar5 = 0;
    vec3 lvar6;
    lvar6 = vec3(lvar5, lvar5, lvar5);
    int lvar7;
    lvar7 = 0;
    while (true) {
        if ((!(lvar7 < r0b0.count))) {
            break;
        }
        vec3 lvar8;
        vec3 lvar9;
        vec3 lvar10;
        float lvar11;
        float lvar12;
        lvar12 = 0.0001;
        dot((r0b0.lights[lvar7].position - lin0), (r0b0.lights[lvar7].position - lin0));
        max(dot((r0b0.lights[lvar7].position - lin0), (r0b0.lights[lvar7].position - lin0)), lvar12);
        normalize((r0b0.lights[lvar7].position - lin0));
        float lvar13;
        lvar13 = 0;
        dot(lin1, normalize((r0b0.lights[lvar7].position - lin0)));
        max(dot(lin1, normalize((r0b0.lights[lvar7].position - lin0))), lvar13);
        lvar6 = (lvar6 + (((texture(r0b0, lin2).xyz * max(dot(lin1, normalize((r0b0.lights[lvar7].position - lin0))), lvar13)) * r0b0.lights[lvar7].color) * (r0b0.lights[lvar7].intensity / max(dot((r0b0.lights[lvar7].position - lin0), (r0b0.lights[lvar7].position - lin0)), lvar12))));
        int lvar14;
        lvar14 = 1;
        lvar7 = (lvar7 + lvar14);
    }
    lout0 = lvar6;
}
