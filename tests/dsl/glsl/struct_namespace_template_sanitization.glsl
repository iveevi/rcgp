#version 460

#extension GL_EXT_scalar_block_layout : require

struct sanitize_case_value_or_index_scalar_float {
    float value;
    int index;
};

struct sanitize_case_value_or_index_vector_float_3 {
    vec3 value;
    int index;
};

struct sanitize_case_GenericMaterialInterface_Encoder {
    sanitize_case_value_or_index_vector_float_3 albedo;
    sanitize_case_value_or_index_vector_float_3 specular;
    sanitize_case_value_or_index_scalar_float roughness;
};

layout (location = 0) smooth in vec2 lin0;

layout (location = 0) out vec4 lout0;

layout (std430, push_constant) uniform PC {
    layout (offset = 0) sanitize_case_GenericMaterialInterface_Encoder pc;
};

void main()
{
    vec3 lvar0;
    lvar0 = vec3(lin0, pc.roughness.value);
    vec3 lvar1;
    lvar1 = (pc.albedo.value + lvar0);
    float lvar2;
    lvar2 = 1;
    vec4 lvar3;
    lvar3 = vec4(lvar1, lvar2);
    lout0 = lvar3;
}
