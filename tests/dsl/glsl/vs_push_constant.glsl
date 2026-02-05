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
    vec3 lvar0;
    mat4 lvar1;
    mat4 lvar2;
    mat4 lvar3;
    float lvar4;
    lvar4 = 1;
    vec4 lvar5;
    lvar5 = vec4(lin0, lvar4);
    vec4 lvar6;
    lvar6 = (pc.model * lvar5);
    mat4 lvar7;
    lvar7 = (pc.proj * pc.view);
    vec4 lvar8;
    lvar8 = (lvar7 * lvar6);
    gl_Position = lvar8;
    vec3 lvar9;
    lvar9 = vec3(lvar6);
    lout0 = lvar9;
}
