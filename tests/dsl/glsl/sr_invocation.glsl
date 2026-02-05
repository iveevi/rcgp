#version 460

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
    float lvar0;
    vec3 lvar1;
    lvar1 = vec3(arg0, arg0, arg0);
    ret0 = lvar1;
}

void sr2(float arg0, uint arg1, out vec3 ret0, out uvec2 ret1)
{
    uint lvar2;
    float lvar3;
    vec3 lvar4;
    lvar4 = vec3(arg0, arg0, arg0);
    uint lvar5;
    lvar5 = 13;
    uvec2 lvar6;
    lvar6 = uvec2(arg1, lvar5);
    ret0 = lvar4;
    ret1 = lvar6;
}

void sr3(float arg0, out Ray ret0)
{
    float lvar7;
    float lvar8;
    lvar8 = 0;
    vec3 lvar9;
    lvar9 = vec3(lvar8, lvar8, lvar8);
    float lvar10;
    lvar10 = 1;
    float lvar11;
    lvar11 = 1;
    vec3 lvar12;
    lvar12 = vec3(lvar11, arg0, lvar10);
    normalize(lvar12);
    ret0 = Ray(lvar9, normalize(lvar12));
}

void main()
{
    float lvar13;
    lvar13 = 1;
    vec3 lvar14;
    sr1(lvar13, lvar14);
    vec3 lvar15;
    uint lvar16;
    lvar16 = 2;
    float lvar17;
    lvar17 = 1;
    vec3 lvar18;
    uvec2 lvar19;
    sr2(lvar17, lvar16, lvar18, lvar19);
    uvec2 lvar20;
    vec3 lvar21;
    float lvar22;
    lvar22 = 2;
    Ray lvar23;
    sr3(lvar22, lvar23);
    vec3 lvar24;
    vec3 lvar25;
    lout0 = lvar14;
    lout1 = lvar19;
    lout2 = lvar18;
    lout3 = lvar23.origin;
    lout4 = lvar23.direction;
}
