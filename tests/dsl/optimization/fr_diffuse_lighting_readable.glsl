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

layout (std430, set = 0, binding = 0) readonly buffer Buffer0x0 {
    int count;
    fwd_PointLight lights[];
} r0b0;

layout (set = 0, binding = 0) uniform sampler2D r0b0;

void main()
{
    vec3 lvar0;
    lvar0 = vec3(0, 0, 0);
    int lvar1;
    lvar1 = 0;
    while (true) {
        bool lvar2 = (!(lvar1 < r0b0.count));
        if (lvar2) {
            break;
        }
        fwd_PointLight lvar3 = r0b0.lights[lvar1];
        vec3 lvar4 = (lvar3.position - lin0);
        float lvar5 = max(dot(lvar4, lvar4), 0.0001);
        float lvar6 = max(dot(lin1, normalize(lvar4)), 0);
        vec3 lvar7 = (texture(r0b0, lin2).xyz * lvar6);
        vec3 lvar8 = ((lvar7 * lvar3.color) * (lvar3.intensity / lvar5));
        lvar0 = (lvar0 + lvar8);
        lvar1 = (lvar1 + 1);
    }
    lout0 = lvar0;
}
