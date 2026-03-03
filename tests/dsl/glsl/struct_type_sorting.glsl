#version 460

#extension GL_EXT_scalar_block_layout : require

struct struct_sort_case_ZLeaf {
    vec3 value;
    int index;
};

struct struct_sort_case_AParent {
    struct_sort_case_ZLeaf payload;
    float weight;
};

layout (location = 0) smooth in vec2 lin0;

layout (location = 0) out vec4 lout0;

layout (std430, push_constant) uniform PC {
    layout (offset = 0) struct_sort_case_AParent pc;
};

void main()
{
    vec3 lvar0;
    lvar0 = vec3(lin0, pc.weight);
    vec3 lvar1;
    lvar1 = (pc.payload.value + lvar0);
    float lvar2;
    lvar2 = 1;
    vec4 lvar3;
    lvar3 = vec4(lvar1, lvar2);
    lout0 = lvar3;
}
