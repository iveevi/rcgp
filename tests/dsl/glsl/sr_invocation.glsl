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
    vec3 lvar13;
    lvar13 = normalize(lvar12);
    ret0 = Ray(lvar9, lvar13);
}

void main()
{
    float lvar14;
    lvar14 = 1;
    vec3 lvar15;
    sr1(lvar14, lvar15);
    vec3 lvar16;
    uint lvar17;
    lvar17 = 2;
    float lvar18;
    lvar18 = 1;
    vec3 lvar19;
    uvec2 lvar20;
    sr2(lvar18, lvar17, lvar19, lvar20);
    uvec2 lvar21;
    vec3 lvar22;
    float lvar23;
    lvar23 = 2;
    Ray lvar24;
    sr3(lvar23, lvar24);
    vec3 lvar25;
    vec3 lvar26;
    lout0 = lvar15;
    lout1 = lvar20;
    lout2 = lvar19;
    lout3 = lvar24.origin;
    lout4 = lvar24.direction;
}
