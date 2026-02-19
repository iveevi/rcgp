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
    SetMeshOutputsEXT(r0b0.value[task_payload.meshlet].vertex_count, r0b0.value[task_payload.meshlet].primitive_count);
    uint lvar0;
    lvar0 = 0;
    while (true) {
        if ((!(lvar0 < r0b0.value[task_payload.meshlet].vertex_count))) {
            break;
        }
        gl_MeshVerticesEXT[lvar0].gl_Position = (pc.view_proj * r0b0.value[r0b0.value[(r0b0.value[task_payload.meshlet].vertex_offset + lvar0)]]);
        lout0[lvar0] = r0b0.value[task_payload.meshlet];
        lvar0 = (lvar0 + 1);
    }
    uint lvar1;
    lvar1 = 0;
    while (true) {
        if ((!(lvar1 < r0b0.value[task_payload.meshlet].primitive_count))) {
            break;
        }
        gl_PrimitiveTriangleIndicesEXT[lvar1] = r0b0.value[(r0b0.value[task_payload.meshlet].primitive_offset + lvar1)];
        lvar1 = (lvar1 + 1);
    }
}
