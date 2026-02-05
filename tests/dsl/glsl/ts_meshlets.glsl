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
    uint lvar0;
    uint lvar1;
    mat4 lvar2;
    uint lvar3;
    uint lvar4;
    mat4 lvar5;
    uint lvar6;
    if ((((gl_WorkGroupID.y * pc.task_group_width) + gl_WorkGroupID.x) < pc.meshlet_count)) {
        uint lvar7;
        uint lvar8;
        uint lvar9;
        uint lvar10;
        vec4 lvar11;
        vec3 lvar12;
        lvar12 = vec3(r0b0.value[((gl_WorkGroupID.y * pc.task_group_width) + gl_WorkGroupID.x)].bounds);
        bool lvar13;
        lvar13 = true;
        uint lvar14;
        lvar14 = 0;
        while (true) {
            uint lvar15;
            lvar15 = 6;
            if ((!(lvar14 < lvar15))) {
                break;
            }
            vec4 lvar16;
            vec3 lvar17;
            lvar17 = vec3(pc.frustum_planes[lvar14]);
            dot(lvar17, lvar12);
            float lvar18;
            lvar18 = -1;
            if (((dot(lvar17, lvar12) + pc.frustum_planes[lvar14].w) < (lvar18 * r0b0.value[((gl_WorkGroupID.y * pc.task_group_width) + gl_WorkGroupID.x)].bounds.w))) {
                bool lvar19;
                lvar19 = false;
                lvar13 = lvar19;
            }
            uint lvar20;
            lvar20 = 1;
            lvar14 = (lvar14 + lvar20);
        }
        if (lvar13) {
            task_payload.meshlet = ((gl_WorkGroupID.y * pc.task_group_width) + gl_WorkGroupID.x);
            uint lvar21;
            lvar21 = 1;
            uint lvar22;
            lvar22 = 1;
            uint lvar23;
            lvar23 = 1;
            EmitMeshTasksEXT(lvar23, lvar22, lvar21);
        }
    }
}
