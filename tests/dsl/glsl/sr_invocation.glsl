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
    vec3 lvar0;
    lvar0 = vec3(arg0, arg0, arg0);
    ret0 = lvar0;
}

void sr2(float arg0, uint arg1, out vec3 ret0, out uvec2 ret1)
{
    vec3 lvar1;
    lvar1 = vec3(arg0, arg0, arg0);
    uint lvar2;
    lvar2 = 13;
    uvec2 lvar3;
    lvar3 = uvec2(arg1, lvar2);
    ret0 = lvar1;
    ret1 = lvar3;
}

void sr3(float arg0, out Ray ret0)
{
    float lvar4;
    lvar4 = 0;
    vec3 lvar5;
    lvar5 = vec3(lvar4, lvar4, lvar4);
    float lvar6;
    lvar6 = 1;
    float lvar7;
    lvar7 = 1;
    vec3 lvar8;
    lvar8 = vec3(lvar7, arg0, lvar6);
    vec3 lvar9;
    lvar9 = normalize(lvar8);
    ret0 = Ray(lvar5, lvar9);
}

void main()
{
    float lvar10;
    lvar10 = 1;
    vec3 lvar11;
    sr1(lvar10, lvar11);
    uint lvar12;
    lvar12 = 2;
    float lvar13;
    lvar13 = 1;
    vec3 lvar14;
    uvec2 lvar15;
    sr2(lvar13, lvar12, lvar14, lvar15);
    float lvar16;
    lvar16 = 2;
    Ray lvar17;
    sr3(lvar16, lvar17);
    lout0 = lvar11;
    lout1 = lvar15;
    lout2 = lvar14;
    lout3 = lvar17.origin;
    lout4 = lvar17.direction;
}
