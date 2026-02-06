#pragma once

namespace rcgp {

enum class OperationCode {
	eAdd,          // @glsl:+
	eSubtract,     // @glsl:-
	eMultiply,     // @glsl:*
	eDivide,       // @glsl:/
	eEqual,        // @glsl:==
	eNotEqual,     // @glsl:!=
	eLess,         // @glsl:<
	eLessEqual,    // @glsl:<=
	eGreater,      // @glsl:>
	eGreaterEqual, // @glsl:>=
	eLogicalAnd,   // @glsl:&&
	eLogicalOr,    // @glsl:||
	eLogicalXor,   // @glsl:^
	eLogicalNot,   // @glsl:!
	eBitAnd,       // @glsl:&
	eBitOr,        // @glsl:|
	eBitXor,       // @glsl:^
	eBitNot,       // @glsl:~
	eShiftLeft,    // @glsl:<<
	eShiftRight,   // @glsl:>>
	eMod,          // @glsl:%
};

const char *repr(OperationCode value);
const char *repr_glsl(OperationCode value);

enum class Primitive {
	eBool,    // @glsl:bool
	eInt32,   // @glsl:int
	eUInt32,  // @glsl:uint
	eFloat,   // @glsl:float
	eUVec2,   // @glsl:uvec2
	eUVec3,   // @glsl:uvec3
	eUVec4,   // @glsl:uvec4
	eIVec2,   // @glsl:ivec2
	eIVec3,   // @glsl:ivec3
	eIVec4,   // @glsl:ivec4
	eVec2,    // @glsl:vec2
	eVec3,    // @glsl:vec3
	eVec4,    // @glsl:vec4
	eIMat2x2, // @glsl:imat2
	eIMat2x3, // @glsl:imat2x3
	eIMat2x4, // @glsl:imat2x4
	eIMat3x2, // @glsl:imat3x2
	eIMat3x3, // @glsl:imat3
	eIMat3x4, // @glsl:imat3x4
	eIMat4x2, // @glsl:imat4x2
	eIMat4x3, // @glsl:imat4x3
	eIMat4x4, // @glsl:imat4
	eUMat2x2, // @glsl:umat2
	eUMat2x3, // @glsl:umat2x3
	eUMat2x4, // @glsl:umat2x4
	eUMat3x2, // @glsl:umat3x2
	eUMat3x3, // @glsl:umat3
	eUMat3x4, // @glsl:umat3x4
	eUMat4x2, // @glsl:umat4x2
	eUMat4x3, // @glsl:umat4x3
	eUMat4x4, // @glsl:umat4
	eFMat2x2, // @glsl:mat2
	eFMat2x3, // @glsl:mat2x3
	eFMat2x4, // @glsl:mat2x4
	eFMat3x2, // @glsl:mat3x2
	eFMat3x3, // @glsl:mat3
	eFMat3x4, // @glsl:mat3x4
	eFMat4x2, // @glsl:mat4x2
	eFMat4x3, // @glsl:mat4x3
	eFMat4x4, // @glsl:mat4
};

const char *repr(Primitive value);
const char *repr_glsl(Primitive value);

enum class GlobalResourceKind {
	ePushConstant,
	eUniformBuffer,
	eStorageBuffer,
	eSampler,
};

const char *repr(GlobalResourceKind value);

enum class GlobalResourceLayout {
	eNone,   // @glsl:?
	eScalar, // @glsl:scalar
	eStd430, // @glsl:std430
};

const char *repr(GlobalResourceLayout value);
const char *repr_glsl(GlobalResourceLayout value);

enum class GlobalResourceAccess {
	eRead,
	eWrite,
	eReadWrite,
};

const char *repr(GlobalResourceAccess value);

enum class SystemValue {
	eClipPosition,              // @glsl:gl_Position
	eInstanceIndex,             // @glsl:gl_InstanceIndex
	eVertexIndex,               // @glsl:gl_VertexIndex
	eLocalInvocationID,         // @glsl:gl_LocalInvocationID
	eWorkGroupID,               // @glsl:gl_WorkGroupID
	eGlobalInvocationID,        // @glsl:gl_GlobalInvocationID
	eTaskPayload,               // @glsl:task_payload
	eMeshVertices,              // @glsl:gl_MeshVerticesEXT
	ePrimitiveTriangleIndices,  // @glsl:gl_PrimitiveTriangleIndicesEXT
};

const char *repr(SystemValue value);
const char *repr_glsl(SystemValue value);

enum class RateProperties {
	eSmooth,        // @glsl:smooth
	eFlat,          // @glsl:flat
	eNoPerspective, // @glsl:noperspective
};

const char *repr(RateProperties value);
const char *repr_glsl(RateProperties value);

enum class MeshPrimitive {
	eTriangles,
};

const char *repr(MeshPrimitive value);

enum class BuiltinIntrinsicCode {
	eAbs,               // @glsl:abs
	eCos,               // @glsl:cos
	eCross,             // @glsl:cross
	eDFdx,              // @glsl:dFdx
	eDFdxFine,          // @glsl:dFdxFine
	eDFdy,              // @glsl:dFdy
	eDFdyFine,          // @glsl:dFdyFine
	eDot,               // @glsl:dot
	eInverse,           // @glsl:inverse
	eLength,            // @glsl:length
	eMax,               // @glsl:max
	ePow,               // @glsl:pow
	eToFloat,
	eMin,               // @glsl:min
	eNormalize,         // @glsl:normalize
	eSample,            // @glsl:texture
	eSin,               // @glsl:sin
	eTan,               // @glsl:tan
	eTranspose,         // @glsl:transpose
	eSetMeshOutputsEXT, // @glsl:SetMeshOutputsEXT
	eEmitMeshTasksEXT,  // @glsl:EmitMeshTasksEXT
	eBreak,
	eContinue,
	eDiscard,
	eSqrt,              // @glsl:sqrt
	eSelect,
};

const char *repr(BuiltinIntrinsicCode value);
const char *repr_glsl(BuiltinIntrinsicCode value);

enum class SwizzleCode {
	// Level 1
	eX, eY, eZ, eW,

	// Level 2
	eXX, eXY, eXZ, eXW,
	eYX, eYY, eYZ, eYW,
	eZX, eZY, eZZ, eZW,
	eWX, eWY, eWZ, eWW,
	
	// Level 3
	eXXX, eXXY, eXXZ, eXXW, eXYX, eXYY, eXYZ, eXYW, eXZX, eXZY, eXZZ, eXZW, eXWX, eXWY, eXWZ, eXWW,
	eYXX, eYXY, eYXZ, eYXW, eYYX, eYYY, eYYZ, eYYW, eYZX, eYZY, eYZZ, eYZW, eYWX, eYWY, eYWZ, eYWW,
	eZXX, eZXY, eZXZ, eZXW, eZYX, eZYY, eZYZ, eZYW, eZZX, eZZY, eZZZ, eZZW, eZWX, eZWY, eZWZ, eZWW,
	eWXX, eWXY, eWXZ, eWXW, eWYX, eWYY, eWYZ, eWYW, eWZX, eWZY, eWZZ, eWZW, eWWX, eWWY, eWWZ, eWWW,
	
	// Level 4
	eXXXX, eXXXY, eXXXZ, eXXXW, eXXYX, eXXYY, eXXYZ, eXXYW, eXXZX, eXXZY, eXXZZ, eXXZW, eXXWX, eXXWY, eXXWZ, eXXWW,
	eXYXX, eXYXY, eXYXZ, eXYXW, eXYYX, eXYYY, eXYYZ, eXYYW, eXYZX, eXYZY, eXYZZ, eXYZW, eXYWX, eXYWY, eXYWZ, eXYWW,
	eXZXX, eXZXY, eXZXZ, eXZXW, eXZYX, eXZYY, eXZYZ, eXZYW, eXZZX, eXZZY, eXZZZ, eXZZW, eXZWX, eXZWY, eXZWZ, eXZWW,
	eXWXX, eXWXY, eXWXZ, eXWXW, eXWYX, eXWYY, eXWYZ, eXWYW, eXWZX, eXWZY, eXWZZ, eXWZW, eXWWX, eXWWY, eXWWZ, eXWWW,
	
	eYXXX, eYXXY, eYXXZ, eYXXW, eYXYX, eYXYY, eYXYZ, eYXYW, eYXZX, eYXZY, eYXZZ, eYXZW, eYXWX, eYXWY, eYXWZ, eYXWW,
	eYYXX, eYYXY, eYYXZ, eYYXW, eYYYX, eYYYY, eYYYZ, eYYYW, eYYZX, eYYZY, eYYZZ, eYYZW, eYYWX, eYYWY, eYYWZ, eYYWW,
	eYZXX, eYZXY, eYZXZ, eYZXW, eYZYX, eYZYY, eYZYZ, eYZYW, eYZZX, eYZZY, eYZZZ, eYZZW, eYZWX, eYZWY, eYZWZ, eYZWW,
	eYWXX, eYWXY, eYWXZ, eYWXW, eYWYX, eYWYY, eYWYZ, eYWYW, eYWZX, eYWZY, eYWZZ, eYWZW, eYWWX, eYWWY, eYWWZ, eYWWW,
	
	eZXXX, eZXXY, eZXXZ, eZXXW, eZXYX, eZXYY, eZXYZ, eZXYW, eZXZX, eZXZY, eZXZZ, eZXZW, eZXWX, eZXWY, eZXWZ, eZXWW,
	eZYXX, eZYXY, eZYXZ, eZYXW, eZYYX, eZYYY, eZYYZ, eZYYW, eZYZX, eZYZY, eZYZZ, eZYZW, eZYWX, eZYWY, eZYWZ, eZYWW,
	eZZXX, eZZXY, eZZXZ, eZZXW, eZZYX, eZZYY, eZZYZ, eZZYW, eZZZX, eZZZY, eZZZZ, eZZZW, eZZWX, eZZWY, eZZWZ, eZZWW,
	eZWXX, eZWXY, eZWXZ, eZWXW, eZWYX, eZWYY, eZWYZ, eZWYW, eZWZX, eZWZY, eZWZZ, eZWZW, eZWWX, eZWWY, eZWWZ, eZWWW,
	
	eWXXX, eWXXY, eWXXZ, eWXXW, eWXYX, eWXYY, eWXYZ, eWXYW, eWXZX, eWXZY, eWXZZ, eWXZW, eWXWX, eWXWY, eWXWZ, eWXWW,
	eWYXX, eWYXY, eWYXZ, eWYXW, eWYYX, eWYYY, eWYYZ, eWYYW, eWYZX, eWYZY, eWYZZ, eWYZW, eWYWX, eWYWY, eWYWZ, eWYWW,
	eWZXX, eWZXY, eWZXZ, eWZXW, eWZYX, eWZYY, eWZYZ, eWZYW, eWZZX, eWZZY, eWZZZ, eWZZW, eWZWX, eWZWY, eWZWZ, eWZWW,
	eWWXX, eWWXY, eWWXZ, eWWXW, eWWYX, eWWYY, eWWYZ, eWWYW, eWWZX, eWWZY, eWWZZ, eWWZW, eWWWX, eWWWY, eWWWZ, eWWWW,
};

const char *repr(SwizzleCode value);

enum class ShaderStage {
	eSubroutine,
	eVertex,
	eFragment,
	eCompute,
	eTask,
	eMesh,
};

const char *repr(ShaderStage value);

} // namespace rcgp
