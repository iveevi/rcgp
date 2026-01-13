#pragma once

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
};

enum class GlobalResourceKind {
	ePushConstant,
	eUniformBuffer,
	eStorageBuffer,
	eSampler,
};

enum class GlobalResourceLayout {
	eNone,
	eScalar,
	eStd430,
};

enum class GlobalResourceAccess {
	eRead,
	eWrite,
	eReadWrite,
};

enum class GlobalIntrinsic {
	eScreenPosition,
	eInstanceIndex,
	eVertexIndex,
	eLocalInvocationID,
	eWorkGroupID,
	eGlobalInvocationID,
	eTaskPayload,
	eMeshVertices,
	ePrimitiveTriangleIndices,
};

enum class RateProperties {
	eNone,
	eSmooth,
	eFlat,
	eNoPerspective,
};

enum class MeshPrimitive {
	eTriangles,
};

enum class BuiltinIntrinsicCode {
	eCos,
	eCross,
	eDFdx,
	eDFdxFine,
	eDFdy,
	eDFdyFine,
	eDot,
	eInverse,
	eMax,
	ePow,
	eMin,
	eNormalize,
	eSample,
	eSin,
	eTan,
	eTranspose,
	eSetMeshOutputsEXT,
	eEmitMeshTasksEXT,
};

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

enum class ShaderStage {
	eSubroutine,
	eVertex,
	eFragment,
	eCompute,
	eTask,
	eMesh,
};

enum class LoopKind {
	eWhile,
	eFor,
};
