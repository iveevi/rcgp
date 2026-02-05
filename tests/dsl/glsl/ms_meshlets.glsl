#version 460

#extension GL_EXT_scalar_block_layout : require
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

layout (location = 0) smooth out vec3 lout0[64];

layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout (max_vertices = 64, max_primitives = 126) out;
layout (triangles) out;

taskPayloadSharedEXT meshletsxTaskPayloadData task_payload;

layout (std430, push_constant) uniform PC {
    layout (offset = 0) meshletsxViewData pc;
};

layout (std430, set = 0, binding = 0) readonly buffer Buffer0x0 {
    vec4 value[];
} r0b0;

layout (std430, set = 0, binding = 0) readonly buffer Buffer0x0 {
    meshletsxMeshletData value[];
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
    uint lvar0;
    uint lvar1;
    uint lvar2;
    mat4 lvar3;
    uint lvar4;
    uint lvar5;
    uint lvar6;
    uint lvar7;
    vec4 lvar8;
    SetMeshOutputsEXT(r0b0.value[task_payload.meshlet].vertex_count, r0b0.value[task_payload.meshlet].primitive_count);
    uint lvar9;
    lvar9 = 0;
    while (true) {
        bool lvar10;
        lvar10 = (lvar9 < r0b0.value[task_payload.meshlet].vertex_count);
        if ((!lvar10)) {
            break;
        }
        uint lvar11;
        vec4 lvar12;
        vec4 lvar13;
        lvar13 = (pc.view_proj * r0b0.value[r0b0.value[(r0b0.value[task_payload.meshlet].vertex_offset + lvar9)]]);
        vec4 lvar14;
        gl_MeshVerticesEXT[lvar9].gl_Position = lvar13;
        vec3 lvar15;
        vec3 lvar16;
        lout0[lvar9] = r0b0.value[task_payload.meshlet];
        uint lvar17;
        lvar17 = 1;
        lvar9 = (lvar9 + lvar17);
    }
    uint lvar18;
    lvar18 = 0;
    while (true) {
        bool lvar19;
        lvar19 = (lvar18 < r0b0.value[task_payload.meshlet].primitive_count);
        if ((!lvar19)) {
            break;
        }
        uvec3 lvar20;
        uvec3 lvar21;
        gl_PrimitiveTriangleIndicesEXT[lvar18] = r0b0.value[(r0b0.value[task_payload.meshlet].primitive_offset + lvar18)];
        uint lvar22;
        lvar22 = 1;
        lvar18 = (lvar18 + lvar22);
    }
}
