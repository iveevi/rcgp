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
    uint lvar0 = task_payload.meshlet;
    meshlets_MeshletData lvar1 = r0b0.value[lvar0];
    uint lvar2 = lvar1.vertex_count;
    uint lvar3 = lvar1.primitive_count;
    SetMeshOutputsEXT(lvar2, lvar3);
    uint lvar4;
    lvar4 = 0;
    while (true) {
        if ((!(lvar4 < lvar2))) {
            break;
        }
        gl_MeshVerticesEXT[lvar4].gl_Position = (pc.view_proj * r0b0.value[r0b0.value[(lvar1.vertex_offset + lvar4)]]);
        lout0[lvar4] = r0b0.value[lvar0];
        lvar4 = (lvar4 + 1);
    }
    uint lvar5;
    lvar5 = 0;
    while (true) {
        if ((!(lvar5 < lvar3))) {
            break;
        }
        gl_PrimitiveTriangleIndicesEXT[lvar5] = r0b0.value[(lvar1.primitive_offset + lvar5)];
        lvar5 = (lvar5 + 1);
    }
}
