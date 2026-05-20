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
    uint lvar0;
    lvar0 = pc.meshlet_count;
    uint lvar1;
    lvar1 = pc.task_group_width;
    uvec3 lvar2;
    lvar2 = gl_WorkGroupID;
    uint lvar3;
#ifdef __clang__
    lvar3 = lvar2.y;
#elif defined(__GNUC__)
    lvar3 = lvar2.x;
#endif
    uint lvar4;
#ifdef __clang__
    lvar4 = (lvar3 * lvar1);
#elif defined(__GNUC__)
    lvar4 = lvar2.y;
#endif
    uint lvar5;
#ifdef __clang__
    lvar5 = lvar2.x;
#elif defined(__GNUC__)
    lvar5 = (lvar4 * lvar1);
#endif
    uint lvar6;
#ifdef __clang__
    lvar6 = (lvar4 + lvar5);
#elif defined(__GNUC__)
    lvar6 = (lvar5 + lvar3);
#endif
    uint lvar7;
#ifdef __clang__
    lvar7 = lvar6;
#elif defined(__GNUC__)
    lvar7 = lvar0;
#endif
    uint lvar8;
#ifdef __clang__
    lvar8 = lvar0;
#elif defined(__GNUC__)
    lvar8 = lvar6;
#endif
    bool lvar9;
#ifdef __clang__
    lvar9 = (lvar7 < lvar8);
#elif defined(__GNUC__)
    lvar9 = (lvar8 < lvar7);
#endif
    bool lvar10;
    lvar10 = lvar9;
    bool lvar11;
    lvar11 = lvar10;
    if (lvar11) {
        uint lvar12;
        lvar12 = lvar6;
        vec4 lvar13;
        lvar13 = vec4(r0b0.value[lvar12].bounds);
        vec3 lvar14;
        lvar14 = vec3(lvar13);
        float lvar15;
        lvar15 = r0b0.value[lvar12].bounds.w;
        bool lvar16;
        lvar16 = true;
        uint lvar17;
        lvar17 = 0;
        while (true) {
            uint lvar18;
#ifdef __clang__
            lvar18 = lvar17;
#elif defined(__GNUC__)
            lvar18 = 6;
#endif
            uint lvar19;
#ifdef __clang__
            lvar19 = 6;
#elif defined(__GNUC__)
            lvar19 = lvar17;
#endif
            bool lvar20;
#ifdef __clang__
            lvar20 = (lvar18 < lvar19);
#elif defined(__GNUC__)
            lvar20 = (lvar19 < lvar18);
#endif
            bool lvar21;
            lvar21 = (!lvar20);
            bool lvar22;
            lvar22 = lvar21;
            bool lvar23;
            lvar23 = lvar22;
            if (lvar23) {
                break;
            }
            uint lvar24;
            lvar24 = lvar17;
            vec4 lvar25;
            lvar25 = vec4(pc.frustum_planes[lvar24]);
            vec3 lvar26;
            lvar26 = vec3(lvar25);
            float lvar27;
#ifdef __clang__
            lvar27 = dot(lvar26, lvar14);
#elif defined(__GNUC__)
            lvar27 = pc.frustum_planes[lvar24].w;
#endif
            float lvar28;
#ifdef __clang__
            lvar28 = pc.frustum_planes[lvar24].w;
#elif defined(__GNUC__)
            lvar28 = dot(lvar26, lvar14);
#endif
            float lvar29;
#ifdef __clang__
            lvar29 = (lvar27 + lvar28);
#elif defined(__GNUC__)
            lvar29 = (lvar28 + lvar27);
#endif
            float lvar30;
            lvar30 = -1;
            float lvar31;
            lvar31 = (lvar30 * lvar15);
            float lvar32;
#ifdef __clang__
            lvar32 = lvar29;
#elif defined(__GNUC__)
            lvar32 = lvar31;
#endif
            float lvar33;
#ifdef __clang__
            lvar33 = lvar31;
#elif defined(__GNUC__)
            lvar33 = lvar29;
#endif
            bool lvar34;
#ifdef __clang__
            lvar34 = (lvar32 < lvar33);
#elif defined(__GNUC__)
            lvar34 = (lvar33 < lvar32);
#endif
            bool lvar35;
            lvar35 = lvar34;
            bool lvar36;
            lvar36 = lvar35;
            if (lvar36) {
                bool lvar37;
                lvar37 = false;
                lvar16 = lvar37;
            }
            uint lvar38;
            lvar38 = lvar17;
            uint lvar39;
            lvar39 = 1;
            uint lvar40;
            lvar40 = (lvar17 + lvar39);
            lvar17 = lvar40;
        }
        bool lvar41;
        lvar41 = lvar16;
        bool lvar42;
        lvar42 = lvar41;
        bool lvar43;
        lvar43 = lvar42;
        if (lvar43) {
            task_payload.meshlet = lvar6;
            uint lvar44;
            lvar44 = 1;
            uint lvar45;
            lvar45 = 1;
            uint lvar46;
            lvar46 = 1;
#ifdef __clang__
            EmitMeshTasksEXT(lvar44, lvar45, lvar46);
#elif defined(__GNUC__)
            EmitMeshTasksEXT(lvar46, lvar45, lvar44);
#endif
        }
    }
}
