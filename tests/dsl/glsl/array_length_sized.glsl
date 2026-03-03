#version 460

#extension GL_EXT_scalar_block_layout : require

struct meshlets_ViewData {
    mat4 view_proj;
    vec4 frustum_planes[6];
    uint meshlet_count;
    uint task_group_width;
};

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (std430, push_constant) uniform PC {
    layout (offset = 0) meshlets_ViewData pc;
};

void main()
{
    int lvar0;
    lvar0 = 6;
}
