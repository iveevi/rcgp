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
    vec3 lvar0;
    lvar0 = vec3(0, 0, 0);
    int lvar1;
    lvar1 = 0;
    while (true) {
        if ((!(lvar1 < r0b0.count))) {
            break;
        }
        lvar0 = (lvar0 + (((texture(r0b0, lin2).xyz * max(dot(lin1, normalize((r0b0.lights[lvar1].position - lin0))), 0)) * r0b0.lights[lvar1].color) * (r0b0.lights[lvar1].intensity / max(dot((r0b0.lights[lvar1].position - lin0), (r0b0.lights[lvar1].position - lin0)), 0.0001))));
        lvar1 = (lvar1 + 1);
    }
    lout0 = lvar0;
}
