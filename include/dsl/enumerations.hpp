#pragma once

#include <string_view>

namespace rcgp {

// TODO: refactor repr file to pygen_enumerations_repr.cpp
enum class OperationCode {
	eAdd,
	eSubtract,
	eMultiply,
	eDivide,
	eEqual,
	eNotEqual,
	eLess,
	eLessEqual,
	eGreater,
	eGreaterEqual,
	eLogicalAnd,
	eLogicalOr,
	eLogicalXor,
	eLogicalNot,
	eBitAnd,
	eBitOr,
	eBitXor,
	eBitNot,
	eShiftLeft,
	eShiftRight,
};

std::string_view repr(OperationCode value);

enum class Primitive {
	eBool,
	eInt32,
	eUInt32,
	eFloat,
	eUVec2,
	eUVec3,
	eUVec4,
	eIVec2,
	eIVec3,
	eIVec4,
	eVec2,
	eVec3,
	eVec4,
	eIMat2x2,
	eIMat2x3,
	eIMat2x4,
	eIMat3x2,
	eIMat3x3,
	eIMat3x4,
	eIMat4x2,
	eIMat4x3,
	eIMat4x4,
	eUMat2x2,
	eUMat2x3,
	eUMat2x4,
	eUMat3x2,
	eUMat3x3,
	eUMat3x4,
	eUMat4x2,
	eUMat4x3,
	eUMat4x4,
	eFMat2x2,
	eFMat2x3,
	eFMat2x4,
	eFMat3x2,
	eFMat3x3,
	eFMat3x4,
	eFMat4x2,
	eFMat4x3,
	eFMat4x4,
};

std::string_view repr(Primitive value);

enum class GlobalResourceKind {
	ePushConstant,
	eUniformBuffer,
	eStorageBuffer,
	eSampler,
};

std::string_view repr(GlobalResourceKind value);

enum class GlobalResourceLayout {
	eNone,
	eScalar,
	eStd430,
};

std::string_view repr(GlobalResourceLayout value);

enum class GlobalResourceAccess {
	eRead,
	eWrite,
	eReadWrite,
};

std::string_view repr(GlobalResourceAccess value);

enum class SystemValue {
	eClipPosition,
	eInstanceIndex,
	eVertexIndex,
	eLocalInvocationID,
	eWorkGroupID,
	eGlobalInvocationID,
	eTaskPayload,
	eMeshVertices,
	ePrimitiveTriangleIndices,
};

std::string_view repr(SystemValue value);

enum class RateProperties {
	eNone,
	eSmooth,
	eFlat,
	eNoPerspective,
};

std::string_view repr(RateProperties value);

enum class MeshPrimitive {
	eTriangles,
};

std::string_view repr(MeshPrimitive value);

enum class BuiltinIntrinsicCode {
	eAbs,
	eCos,
	eCross,
	eDFdx,
	eDFdxFine,
	eDFdy,
	eDFdyFine,
	eDot,
	eInverse,
	eLength,
	eMax,
	ePow,
	eToFloat,
	eMin,
	eNormalize,
	eSample,
	eSin,
	eTan,
	eTranspose,
	eSetMeshOutputsEXT,
	eEmitMeshTasksEXT,
	eBreak,
	eContinue,
	eDiscard,
};

std::string_view repr(BuiltinIntrinsicCode value);

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

std::string_view repr(SwizzleCode value);

enum class ShaderStage {
	eSubroutine,
	eVertex,
	eFragment,
	eCompute,
	eTask,
	eMesh,
};

std::string_view repr(ShaderStage value);

} // namespace rcgp
