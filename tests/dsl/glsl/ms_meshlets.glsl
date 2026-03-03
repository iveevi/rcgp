#version 460

#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_mesh_shader : require

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

layout (location = 0) smooth out vec3 lout0[64];

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (max_vertices = 64, max_primitives = 126) out;
layout (triangles) out;

taskPayloadSharedEXT meshlets_TaskPayloadData task_payload;

layout (std430, push_constant) uniform PC {
    layout (offset = 0) meshlets_ViewData pc;
};

layout (std430, set = 0, binding = 0) readonly buffer Buffer0x0 {
    vec4 value[];
} r0b0;

layout (std430, set = 0, binding = 0) readonly buffer Buffer0x0 {
    meshlets_MeshletData value[];
} r0b0;

layout (std430, set = 0, binding = 0) readonly buffer Buffer0x0 {
    uint value[];
} r0b0;

layout (scalar, set = 0, binding = 0) readonly buffer Buffer0x0 {
    uvec3 value[];
} r0b0;

layout (scalar, set = 0, binding = 0) readonly buffer Buffer0x0 {
    vec3 value[];
} r0b0;

void main()
{
    SetMeshOutputsEXT(r0b0.value[task_payload.meshlet].vertex_count, r0b0.value[task_payload.meshlet].primitive_count);
    uint lvar0;
    lvar0 = 0;
    while (true) {
        bool lvar1;
        lvar1 = (lvar0 < r0b0.value[task_payload.meshlet].vertex_count);
        bool lvar2;
        lvar2 = (!lvar1);
        if (lvar2) {
            break;
        }
        uint lvar3;
        lvar3 = (r0b0.value[task_payload.meshlet].vertex_offset + lvar0);
        vec4 lvar4;
        lvar4 = (pc.view_proj * r0b0.value[r0b0.value[lvar3]]);
        gl_MeshVerticesEXT[lvar0].gl_Position = lvar4;
        lout0[lvar0] = r0b0.value[task_payload.meshlet];
        uint lvar5;
        lvar5 = 1;
        uint lvar6;
        lvar6 = (lvar0 + lvar5);
        lvar0 = lvar6;
    }
    uint lvar7;
    lvar7 = 0;
    while (true) {
        bool lvar8;
        lvar8 = (lvar7 < r0b0.value[task_payload.meshlet].primitive_count);
        bool lvar9;
        lvar9 = (!lvar8);
        if (lvar9) {
            break;
        }
        uint lvar10;
        lvar10 = (r0b0.value[task_payload.meshlet].primitive_offset + lvar7);
        gl_PrimitiveTriangleIndicesEXT[lvar7] = r0b0.value[lvar10];
        uint lvar11;
        lvar11 = 1;
        uint lvar12;
        lvar12 = (lvar7 + lvar11);
        lvar7 = lvar12;
    }
}
