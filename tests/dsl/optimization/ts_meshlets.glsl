#version 460

#extension GL_EXT_mesh_shader : require

struct meshletsxMeshletData {
    uint vertex_offset;
    uint vertex_count;
    uint primitive_offset;
    uint primitive_count;
    vec4 bounds;
};

struct meshletsxTaskPayloadData {
    uint meshlet;
};

struct meshletsxViewData {
    mat4 view_proj;
    vec4 frustum_planes[6];
    uint meshlet_count;
    uint task_group_width;
};

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

taskPayloadSharedEXT meshletsxTaskPayloadData task_payload;

layout (std430, push_constant) uniform PC {
    layout (offset = 0) meshletsxViewData pc;
};

layout (std430, set = 0, binding = 0) readonly buffer Buffer0x0 {
    meshletsxMeshletData value[];
} r0b0;

void main()
{
    if ((((gl_WorkGroupID.y * pc.task_group_width) + gl_WorkGroupID.x) < pc.meshlet_count)) {
        bool lvar0;
        lvar0 = true;
        uint lvar1;
        lvar1 = 0;
        while (true) {
            if ((!(lvar1 < 6))) {
                break;
            }
            if (((dot(vec3(pc.frustum_planes[lvar1]), vec3(r0b0.value[((gl_WorkGroupID.y * pc.task_group_width) + gl_WorkGroupID.x)].bounds)) + pc.frustum_planes[lvar1].w) < (-1 * r0b0.value[((gl_WorkGroupID.y * pc.task_group_width) + gl_WorkGroupID.x)].bounds.w))) {
                lvar0 = false;
            }
            lvar1 = (lvar1 + 1);
        }
        if (lvar0) {
            task_payload.meshlet = ((gl_WorkGroupID.y * pc.task_group_width) + gl_WorkGroupID.x);
            EmitMeshTasksEXT(1, 1, 1);
        }
    }
}
