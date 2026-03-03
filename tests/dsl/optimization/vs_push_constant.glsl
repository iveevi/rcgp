#version 460

#extension GL_EXT_scalar_block_layout : require

struct fwd_View {
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout (location = 0) in vec3 lin0;

layout (location = 0) smooth out vec3 lout0;

layout (std430, push_constant) uniform PC {
    layout (offset = 0) fwd_View pc;
};

void main()
{
    vec4 lvar0 = (pc.model * vec4(lin0, 1));
    gl_Position = ((pc.proj * pc.view) * lvar0);
    lout0 = vec3(lvar0);
}
