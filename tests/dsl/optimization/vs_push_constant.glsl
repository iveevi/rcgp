#version 460

struct fwdxView {
    mat4 model;
    mat4 view;
    mat4 proj;
};

layout (location = 0) in vec3 lin0;

layout (location = 0) smooth out vec3 lout0;

layout (std430, push_constant) uniform PC {
    layout (offset = 0) fwdxView pc;
};

void main()
{
    gl_Position = ((pc.proj * pc.view) * (pc.model * vec4(lin0, 1)));
    lout0 = vec3((pc.model * vec4(lin0, 1)));
}
