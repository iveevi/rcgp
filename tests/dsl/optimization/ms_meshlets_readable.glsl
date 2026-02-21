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
    uint lvar0 = task_payload.meshlet;
    meshletsxMeshletData lvar1 = r0b0.value[lvar0];
    uint lvar2 = lvar1.vertex_count;
    uint lvar3 = lvar1.primitive_count;
    SetMeshOutputsEXT(lvar2, lvar3);
    uint lvar4;
    lvar4 = 0;
    while (true) {
        if ((!(lvar4 < lvar2))) {
            break;
        }
        uint lvar5 = r0b0.value[(lvar1.vertex_offset + lvar4)];
        vec4 lvar6 = (pc.view_proj * r0b0.value[lvar5]);
        gl_MeshVerticesEXT[lvar4].gl_Position = lvar6;
        lout0[lvar4] = r0b0.value[lvar0];
        lvar4 = (lvar4 + 1);
    }
    uint lvar7;
    lvar7 = 0;
    while (true) {
        if ((!(lvar7 < lvar3))) {
            break;
        }
        uvec3 lvar8 = r0b0.value[(lvar1.primitive_offset + lvar7)];
        gl_PrimitiveTriangleIndicesEXT[lvar7] = lvar8;
        lvar7 = (lvar7 + 1);
    }
}
