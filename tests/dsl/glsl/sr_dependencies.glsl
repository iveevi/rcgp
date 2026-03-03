#version 460

#extension GL_EXT_scalar_block_layout : require

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

void b(float arg0, float arg1, float arg2, out float ret0)
{
    float lvar0;
    lvar0 = (arg0 * arg1);
    float lvar1;
    lvar1 = (lvar0 + arg2);
    ret0 = lvar1;
}

void c(vec2 arg0, vec2 arg1, vec2 arg2, out vec2 ret0)
{
    float lvar2;
    lvar2 = arg2.y;
    float lvar3;
    lvar3 = arg1.y;
    float lvar4;
    lvar4 = arg0.y;
    float lvar5;
    b(lvar4, lvar3, lvar2, lvar5);
    float lvar6;
    lvar6 = arg2.y;
    float lvar7;
    lvar7 = arg1.x;
    float lvar8;
    lvar8 = arg0.x;
    float lvar9;
    b(lvar8, lvar7, lvar6, lvar9);
    vec2 lvar10;
    lvar10 = vec2(lvar9, lvar5);
    ret0 = lvar10;
}

void a(vec4 arg0, vec4 arg1, vec4 arg2, out vec4 ret0)
{
    vec2 lvar11;
    lvar11 = arg2.zw;
    vec2 lvar12;
    lvar12 = arg1.zw;
    vec2 lvar13;
    lvar13 = arg0.zw;
    vec2 lvar14;
    c(lvar13, lvar12, lvar11, lvar14);
    vec2 lvar15;
    lvar15 = arg2.yy;
    vec2 lvar16;
    lvar16 = arg1.xy;
    vec2 lvar17;
    lvar17 = arg0.xy;
    vec2 lvar18;
    c(lvar17, lvar16, lvar15, lvar18);
    vec4 lvar19;
    lvar19 = vec4(lvar18, lvar14);
    ret0 = lvar19;
}

void main()
{
    float lvar20;
    lvar20 = 1;
    vec4 lvar21;
    lvar21 = vec4(lvar20, lvar20, lvar20, lvar20);
    float lvar22;
    lvar22 = 2;
    vec4 lvar23;
    lvar23 = vec4(lvar22, lvar22, lvar22, lvar22);
    float lvar24;
    lvar24 = 3;
    vec4 lvar25;
    lvar25 = vec4(lvar24, lvar24, lvar24, lvar24);
    vec4 lvar26;
    a(lvar25, lvar21, lvar23, lvar26);
}
