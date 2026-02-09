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
    uvec3 lvar0;
    lvar0 = gl_WorkGroupID;
    uint lvar1;
    lvar1 = lvar0.x;
    uint lvar2;
    lvar2 = lvar0.y;
    uint lvar3;
    lvar3 = (lvar2 * pc.task_group_width);
    uint lvar4;
    lvar4 = (lvar3 + lvar1);
    bool lvar5;
    lvar5 = (lvar4 < pc.meshlet_count);
    if (lvar5) {
        vec3 lvar6;
        lvar6 = vec3(r0b0.value[lvar4].bounds);
        float lvar7;
        lvar7 = r0b0.value[lvar4].bounds.w;
        bool lvar8;
        lvar8 = true;
        uint lvar9;
        lvar9 = 0;
        while (true) {
            uint lvar10;
            lvar10 = 6;
            bool lvar11;
            lvar11 = (lvar9 < lvar10);
            bool lvar12;
            lvar12 = (!lvar11);
            if (lvar12) {
                break;
            }
            vec3 lvar13;
            lvar13 = vec3(pc.frustum_planes[lvar9]);
            float lvar14;
            lvar14 = pc.frustum_planes[lvar9].w;
            dot(lvar13, lvar6);
            float lvar15;
            lvar15 = dot(lvar13, lvar6);
            float lvar16;
            lvar16 = (lvar15 + lvar14);
            float lvar17;
            lvar17 = -1;
            float lvar18;
            lvar18 = (lvar17 * lvar7);
            bool lvar19;
            lvar19 = (lvar16 < lvar18);
            if (lvar19) {
                bool lvar20;
                lvar20 = false;
                lvar8 = lvar20;
            }
            uint lvar21;
            lvar21 = 1;
            uint lvar22;
            lvar22 = (lvar9 + lvar21);
            lvar9 = lvar22;
        }
        if (lvar8) {
            task_payload.meshlet = lvar4;
            uint lvar23;
            lvar23 = 1;
            uint lvar24;
            lvar24 = 1;
            uint lvar25;
            lvar25 = 1;
            EmitMeshTasksEXT(lvar25, lvar24, lvar23);
        }
    }
}
