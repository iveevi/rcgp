#version 460

#extension GL_EXT_scalar_block_layout : require

struct Ray {
    vec3 origin;
    vec3 direction;
};

layout (location = 0) smooth out vec3 lout0;
layout (location = 1) smooth out vec3 lout1;
layout (location = 2) smooth out uvec2 lout2;
layout (location = 3) smooth out vec3 lout3;
layout (location = 4) smooth out vec3 lout4;

void sr1(float arg0, out vec3 ret0)
{
    ret0 = vec3(arg0, arg0, arg0);
}

void sr3(float arg0, out Ray ret0)
{
    ret0 = Ray(vec3(0, 0, 0), normalize(vec3(1, arg0, 1)));
}

void sr2(float arg0, uint arg1, out vec3 ret0, out uvec2 ret1)
{
    ret0 = vec3(arg0, arg0, arg0);
    ret1 = uvec2(arg1, 13);
}

void main()
{
    vec3 lvar0;
    sr1(1, lvar0);
    vec3 lvar1;
    uvec2 lvar2;
    sr2(1, 2, lvar1, lvar2);
    Ray lvar3;
    sr3(2, lvar3);
    lout0 = lvar0;
    lout1 = lvar2;
    lout2 = lvar1;
    lout3 = lvar3.origin;
    lout4 = lvar3.direction;
}
