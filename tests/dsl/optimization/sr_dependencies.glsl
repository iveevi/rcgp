#version 460

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

void b(float arg0, float arg1, float arg2, out float ret0)
{
    ret0 = ((arg0 * arg1) + arg2);
}

void c(vec2 arg0, vec2 arg1, vec2 arg2, out vec2 ret0)
{
    float lvar0;
    b(arg0.y, arg1.y, arg2.y, lvar0);
    float lvar1;
    b(arg0.x, arg1.x, arg2.y, lvar1);
    ret0 = vec2(lvar1, lvar0);
}

void a(vec4 arg0, vec4 arg1, vec4 arg2, out vec4 ret0)
{
    vec2 lvar2;
    c(arg0.zw, arg1.zw, arg2.zw, lvar2);
    vec2 lvar3;
    c(arg0.xy, arg1.xy, arg2.yy, lvar3);
    ret0 = vec4(lvar3, lvar2);
}

void main()
{
    vec4 lvar4;
    a(vec4(3, 3, 3, 3), vec4(1, 1, 1, 1), vec4(2, 2, 2, 2), lvar4);
}
