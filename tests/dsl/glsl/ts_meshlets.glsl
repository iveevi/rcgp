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
    uvec3 lvar4;
    lvar4 = gl_WorkGroupID;
    uint lvar5;
    lvar5 = lvar4.x;
    uint lvar6;
    lvar6 = lvar4.y;
    bool lvar7;
    lvar7 = (((lvar6 * pc.task_group_width) + lvar5) < pc.meshlet_count);
    if (lvar7) {
        uint lvar8;
        uint lvar9;
        uint lvar10;
        uint lvar11;
        vec4 lvar12;
        vec3 lvar13;
        lvar13 = vec3(r0b0.value[((lvar6 * pc.task_group_width) + lvar5)].bounds);
        float lvar14;
        lvar14 = r0b0.value[((lvar6 * pc.task_group_width) + lvar5)].bounds.w;
        bool lvar15;
        lvar15 = true;
        uint lvar16;
        lvar16 = 0;
        while (true) {
            uint lvar17;
            lvar17 = 6;
            bool lvar18;
            lvar18 = (lvar16 < lvar17);
            if ((!lvar18)) {
                break;
            }
            vec4 lvar19;
            vec3 lvar20;
            lvar20 = vec3(pc.frustum_planes[lvar16]);
            float lvar21;
            lvar21 = pc.frustum_planes[lvar16].w;
            dot(lvar20, lvar13);
            float lvar22;
            lvar22 = dot(lvar20, lvar13);
            float lvar23;
            lvar23 = -1;
            bool lvar24;
            lvar24 = ((lvar22 + lvar21) < (lvar23 * lvar14));
            if (lvar24) {
                bool lvar25;
                lvar25 = false;
                lvar15 = lvar25;
            }
            uint lvar26;
            lvar26 = 1;
            lvar16 = (lvar16 + lvar26);
        }
        if (lvar15) {
            task_payload.meshlet = ((lvar6 * pc.task_group_width) + lvar5);
            uint lvar27;
            lvar27 = 1;
            uint lvar28;
            lvar28 = 1;
            uint lvar29;
            lvar29 = 1;
            EmitMeshTasksEXT(lvar29, lvar28, lvar27);
        }
    }
}
