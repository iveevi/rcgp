#version 460

#extension GL_EXT_scalar_block_layout : require

struct Ray {
    vec3 f0;
    vec3 f1;
};

layout (location = 0) smooth out vec3 lout0;
layout (location = 1) smooth out vec3 lout1;
layout (location = 2) smooth out uvec2 lout2;
layout (location = 3) smooth out vec3 lout3;
layout (location = 4) smooth out vec3 lout4;

void sr1(float arg0, out vec3 ret0)
{
    float lvar0;
    lvar0 = 1;
    vec3 lvar1;
    sr1(lvar0, lvar1);
    vec3 lvar2;
    uint lvar3;
    lvar3 = 2;
    float lvar4;
    lvar4 = 1;
    vec3 lvar5;
    uvec2 lvar6;
    sr2(lvar4, lvar3, lvar5, lvar6);
    uvec2 lvar7;
    vec3 lvar8;
    float lvar9;
    lvar9 = 2;
    Ray lvar10;
    sr3(lvar9, lvar10);
    vec3 lvar11;
    vec3 lvar12;
    lout0 = lvar1;
    lout1 = lvar6;
    lout2 = lvar5;
    lout3 = lvar10.f0;
    lout4 = lvar10.f1;
}

void sr3(float arg0, out Ray ret0)
{
    float lvar0;
    lvar0 = 1;
    vec3 lvar1;
    sr1(lvar0, lvar1);
    vec3 lvar2;
    uint lvar3;
    lvar3 = 2;
    float lvar4;
    lvar4 = 1;
    vec3 lvar5;
    uvec2 lvar6;
    sr2(lvar4, lvar3, lvar5, lvar6);
    uvec2 lvar7;
    vec3 lvar8;
    float lvar9;
    lvar9 = 2;
    Ray lvar10;
    sr3(lvar9, lvar10);
    vec3 lvar11;
    vec3 lvar12;
    lout0 = lvar1;
    lout1 = lvar6;
    lout2 = lvar5;
    lout3 = lvar10.f0;
    lout4 = lvar10.f1;
}

void sr2(float arg0, uint arg1, out vec3 ret0, out uvec2 ret1)
{
    float lvar0;
    lvar0 = 1;
    vec3 lvar1;
    sr1(lvar0, lvar1);
    vec3 lvar2;
    uint lvar3;
    lvar3 = 2;
    float lvar4;
    lvar4 = 1;
    vec3 lvar5;
    uvec2 lvar6;
    sr2(lvar4, lvar3, lvar5, lvar6);
    uvec2 lvar7;
    vec3 lvar8;
    float lvar9;
    lvar9 = 2;
    Ray lvar10;
    sr3(lvar9, lvar10);
    vec3 lvar11;
    vec3 lvar12;
    lout0 = lvar1;
    lout1 = lvar6;
    lout2 = lvar5;
    lout3 = lvar10.f0;
    lout4 = lvar10.f1;
}

void main()
{
    float lvar0;
    lvar0 = 1;
    vec3 lvar1;
    sr1(lvar0, lvar1);
    vec3 lvar2;
    uint lvar3;
    lvar3 = 2;
    float lvar4;
    lvar4 = 1;
    vec3 lvar5;
    uvec2 lvar6;
    sr2(lvar4, lvar3, lvar5, lvar6);
    uvec2 lvar7;
    vec3 lvar8;
    float lvar9;
    lvar9 = 2;
    Ray lvar10;
    sr3(lvar9, lvar10);
    vec3 lvar11;
    vec3 lvar12;
    lout0 = lvar1;
    lout1 = lvar6;
    lout2 = lvar5;
    lout3 = lvar10.f0;
    lout4 = lvar10.f1;
}
