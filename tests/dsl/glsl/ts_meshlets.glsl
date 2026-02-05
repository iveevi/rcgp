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
    uvec3 lvar7;
    lvar7 = gl_WorkGroupID;
    uint lvar8;
    lvar8 = lvar7.x;
    uint lvar9;
    lvar9 = lvar7.y;
    bool lvar10;
    lvar10 = (((lvar9 * pc.task_group_width) + lvar8) < pc.meshlet_count);
    if (lvar10) {
        uint lvar11;
        uint lvar12;
        uint lvar13;
        uint lvar14;
        vec4 lvar15;
        vec3 lvar16;
        lvar16 = vec3(r0b0.value[((lvar9 * pc.task_group_width) + lvar8)].bounds);
        float lvar17;
        lvar17 = r0b0.value[((lvar9 * pc.task_group_width) + lvar8)].bounds.w;
        bool lvar18;
        lvar18 = true;
        uint lvar19;
        lvar19 = 0;
        while (true) {
            uint lvar20;
            lvar20 = 6;
            bool lvar21;
            lvar21 = (lvar19 < lvar20);
            if ((!lvar21)) {
                break;
            }
            vec4 lvar22;
            vec3 lvar23;
            lvar23 = vec3(pc.frustum_planes[lvar19]);
            float lvar24;
            lvar24 = pc.frustum_planes[lvar19].w;
            dot(lvar23, lvar16);
            float lvar25;
            lvar25 = dot(lvar23, lvar16);
            float lvar26;
            lvar26 = -1;
            bool lvar27;
            lvar27 = ((lvar25 + lvar24) < (lvar26 * lvar17));
            if (lvar27) {
                bool lvar28;
                lvar28 = false;
                lvar18 = lvar28;
            }
            uint lvar29;
            lvar29 = 1;
            lvar19 = (lvar19 + lvar29);
        }
        if (lvar18) {
            task_payload.meshlet = ((lvar9 * pc.task_group_width) + lvar8);
            uint lvar30;
            lvar30 = 1;
            uint lvar31;
            lvar31 = 1;
            uint lvar32;
            lvar32 = 1;
            EmitMeshTasksEXT(lvar32, lvar31, lvar30);
        }
    }
}
