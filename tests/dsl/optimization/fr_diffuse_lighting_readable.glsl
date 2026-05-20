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
    vec3 lvar0 = texture(r0b0, vec2(lin2)).xyz;
    vec3 lvar1;
    lvar1 = vec3(0, 0, 0);
    int lvar2;
    lvar2 = 0;
    while (true) {
        bool lvar3 = (!(lvar2 < r0b0.count));
        if (lvar3) {
            break;
        }
        fwd_PointLight lvar4 = r0b0.lights[lvar2];
        vec3 lvar5 = (lvar4.position - vec3(lin0));
        float lvar6 = max(dot(lvar5, lvar5), 0.0001);
        float lvar7 = dot(vec3(lin1), normalize(lvar5));
        vec3 lvar8 = (lvar0 * max(lvar7, 0));
        vec3 lvar9 = ((lvar8 * lvar4.color) * (lvar4.intensity / lvar6));
        lvar1 = (lvar1 + lvar9);
        lvar2 = (lvar2 + 1);
    }
    lout0 = lvar1;
}
