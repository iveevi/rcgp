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
    float lvar0;
    lvar0 = 1;
    vec4 lvar1;
    lvar1 = vec4(lin0, lvar0);
    vec4 lvar2;
    lvar2 = (pc.model * lvar1);
    mat4 lvar3;
    lvar3 = (pc.proj * pc.view);
    vec4 lvar4;
    lvar4 = (lvar3 * lvar2);
    gl_Position = lvar4;
    vec3 lvar5;
    lvar5 = vec3(lvar2);
    lout0 = lvar5;
}
