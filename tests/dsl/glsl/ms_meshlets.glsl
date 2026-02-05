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
    mat4 lvar6;
    uint lvar7;
    uint lvar8;
    uint lvar9;
    uint lvar10;
    vec4 lvar11;
    SetMeshOutputsEXT(r0b0.value[task_payload.meshlet].vertex_count, r0b0.value[task_payload.meshlet].primitive_count);
    uint lvar12;
    lvar12 = 0;
    while (true) {
        bool lvar13;
        lvar13 = (lvar12 < r0b0.value[task_payload.meshlet].vertex_count);
        if ((!lvar13)) {
            break;
        }
        uint lvar14;
        vec4 lvar15;
        vec4 lvar16;
        lvar16 = (pc.view_proj * r0b0.value[r0b0.value[(r0b0.value[task_payload.meshlet].vertex_offset + lvar12)]]);
        vec4 lvar17;
        gl_MeshVerticesEXT[lvar12].gl_Position = lvar16;
        vec3 lvar18;
        vec3 lvar19;
        lout0[lvar12] = r0b0.value[task_payload.meshlet];
        uint lvar20;
        lvar20 = 1;
        lvar12 = (lvar12 + lvar20);
    }
    uint lvar21;
    lvar21 = 0;
    while (true) {
        bool lvar22;
        lvar22 = (lvar21 < r0b0.value[task_payload.meshlet].primitive_count);
        if ((!lvar22)) {
            break;
        }
        uvec3 lvar23;
        uvec3 lvar24;
        gl_PrimitiveTriangleIndicesEXT[lvar21] = r0b0.value[(r0b0.value[task_payload.meshlet].primitive_offset + lvar21)];
        uint lvar25;
        lvar25 = 1;
        lvar21 = (lvar21 + lvar25);
    }
}
