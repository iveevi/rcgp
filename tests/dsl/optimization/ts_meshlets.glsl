#version 460

#extension GL_EXT_mesh_shader : require
#extension GL_EXT_scalar_block_layout : require

struct meshlets_MeshletData {
    uint vertex_offset;
    uint vertex_count;
    uint primitive_offset;
    uint primitive_count;
    vec4 bounds;
};

struct meshlets_TaskPayloadData {
    uint meshlet;
};

struct meshlets_ViewData {
    mat4 view_proj;
    vec4 frustum_planes[6];
    uint meshlet_count;
    uint task_group_width;
};

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

taskPayloadSharedEXT meshlets_TaskPayloadData task_payload;

layout (std430, push_constant) uniform PC {
    layout (offset = 0) meshlets_ViewData pc;
};

layout (std430, set = 0, binding = 0) readonly buffer Buffer0x0 {
    meshlets_MeshletData value[];
} r0b0;

void main()
{
    uvec3 lvar0 = gl_WorkGroupID;
    uint lvar1 = ((lvar0.y * pc.task_group_width) + lvar0.x);
    if ((lvar1 < pc.meshlet_count)) {
        meshlets_MeshletData lvar2 = r0b0.value[lvar1];
        vec4 lvar3 = lvar2.bounds;
        bool lvar4;
        lvar4 = true;
        uint lvar5;
        lvar5 = 0;
        while (true) {
            if ((!(lvar5 < 6))) {
                break;
            }
            vec4 lvar6 = pc.frustum_planes[lvar5];
            if (((dot(vec3(vec4(lvar6)), vec3(vec4(lvar3))) + lvar6.w) < (-1 * lvar3.w))) {
                lvar4 = false;
            }
            lvar5 = (lvar5 + 1);
        }
        if (lvar4) {
            task_payload.meshlet = lvar1;
            EmitMeshTasksEXT(1, 1, 1);
        }
    }
}
