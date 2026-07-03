#include <utility>

#include "dsl/block.hpp"
#include "dsl/instructions.hpp"
#include "util/error.hpp"

namespace rcgp {

// Included from optimize.cpp inside namespace rcgp.

Primitive swizzle_type(Primitive base, SwizzleCode swizzle)
{
	const auto result_dim = std::string_view(repr(swizzle)).size();
	assertion(result_dim >= 1 && result_dim <= 4, "swizzle dimension out of range");

	const auto is_in_family = [&](Primitive vec2) {
		const auto offset = std::to_underlying(base) - std::to_underlying(vec2);
		return offset >= 0 && offset <= 2;
	};

	const auto map_family = [&](Primitive scalar, Primitive vec2) {
		if (result_dim == 1)
			return scalar;
		return Primitive(std::to_underlying(vec2) + (result_dim - 2));
	};

	if (is_in_family(Primitive::eUVec2))
		return map_family(Primitive::eUInt32, Primitive::eUVec2);
	if (is_in_family(Primitive::eIVec2))
		return map_family(Primitive::eInt32, Primitive::eIVec2);
	if (is_in_family(Primitive::eVec2))
		return map_family(Primitive::eFloat, Primitive::eVec2);

	fatal("unsupported swizzle type from base {} with {}", repr(base), repr(swizzle));
}

Reference get_or_add_type(const SharedBlockReference &sbr, const Type &request)
{
	for (auto &ref : *sbr) {
		if (ref->is <Type> () and ref->repr() == request.repr())
			return ref;
	}

	return sbr->add(0, request);
}

enum class PrimitiveFamily {
	eBool,
	eInt,
	eUInt,
	eFloat,
	eUnknown,
};

bool primitive_is_vector(Primitive prim)
{
	const auto value = std::to_underlying(prim);
	return (value >= std::to_underlying(Primitive::eUVec2)
			and value <= std::to_underlying(Primitive::eUVec4))
		or (value >= std::to_underlying(Primitive::eIVec2)
			and value <= std::to_underlying(Primitive::eIVec4))
		or (value >= std::to_underlying(Primitive::eVec2)
			and value <= std::to_underlying(Primitive::eVec4));
}

bool primitive_is_matrix(Primitive prim)
{
	const auto value = std::to_underlying(prim);
	return (value >= std::to_underlying(Primitive::eIMat2x2)
			and value <= std::to_underlying(Primitive::eIMat4x4))
		or (value >= std::to_underlying(Primitive::eUMat2x2)
			and value <= std::to_underlying(Primitive::eUMat4x4))
		or (value >= std::to_underlying(Primitive::eFMat2x2)
			and value <= std::to_underlying(Primitive::eFMat4x4));
}

bool primitive_is_scalar(Primitive prim)
{
	return prim == Primitive::eBool
		or prim == Primitive::eInt32
		or prim == Primitive::eUInt32
		or prim == Primitive::eFloat;
}

PrimitiveFamily primitive_family(Primitive prim)
{
	if (prim == Primitive::eBool)
		return PrimitiveFamily::eBool;

	if (prim == Primitive::eInt32
		or (prim >= Primitive::eIVec2 and prim <= Primitive::eIVec4)
		or (prim >= Primitive::eIMat2x2 and prim <= Primitive::eIMat4x4))
		return PrimitiveFamily::eInt;
	if (prim == Primitive::eUInt32
		or (prim >= Primitive::eUVec2 and prim <= Primitive::eUVec4)
		or (prim >= Primitive::eUMat2x2 and prim <= Primitive::eUMat4x4))
		return PrimitiveFamily::eUInt;
	if (prim == Primitive::eFloat
		or (prim >= Primitive::eVec2 and prim <= Primitive::eVec4)
		or (prim >= Primitive::eFMat2x2 and prim <= Primitive::eFMat4x4))
		return PrimitiveFamily::eFloat;

	return PrimitiveFamily::eUnknown;
}

size_t primitive_vector_dimension(Primitive prim)
{
	assertion(primitive_is_scalar(prim) or primitive_is_vector(prim), "expected scalar or vector primitive");
	if (primitive_is_scalar(prim))
		return 1;

	if (prim >= Primitive::eUVec2 and prim <= Primitive::eUVec4)
		return std::to_underlying(prim) - std::to_underlying(Primitive::eUVec2) + 2;
	if (prim >= Primitive::eIVec2 and prim <= Primitive::eIVec4)
		return std::to_underlying(prim) - std::to_underlying(Primitive::eIVec2) + 2;
	if (prim >= Primitive::eVec2 and prim <= Primitive::eVec4)
		return std::to_underlying(prim) - std::to_underlying(Primitive::eVec2) + 2;

	fatal("failed to determine vector dimension for {}", repr(prim));
}

size_t primitive_matrix_columns(Primitive prim)
{
	assertion(primitive_is_matrix(prim), "expected matrix primitive");
	const auto base = [&]() {
		if (prim >= Primitive::eIMat2x2 and prim <= Primitive::eIMat4x4)
			return Primitive::eIMat2x2;
		if (prim >= Primitive::eUMat2x2 and prim <= Primitive::eUMat4x4)
			return Primitive::eUMat2x2;
		return Primitive::eFMat2x2;
	}();

	const auto offset = std::to_underlying(prim) - std::to_underlying(base);
	return static_cast <size_t> (2 + (offset / 3));
}

size_t primitive_matrix_rows(Primitive prim)
{
	assertion(primitive_is_matrix(prim), "expected matrix primitive");
	const auto base = [&]() {
		if (prim >= Primitive::eIMat2x2 and prim <= Primitive::eIMat4x4)
			return Primitive::eIMat2x2;
		if (prim >= Primitive::eUMat2x2 and prim <= Primitive::eUMat4x4)
			return Primitive::eUMat2x2;
		return Primitive::eFMat2x2;
	}();

	const auto offset = std::to_underlying(prim) - std::to_underlying(base);
	return static_cast <size_t> (2 + (offset % 3));
}

Primitive scalar_primitive(PrimitiveFamily family)
{
	switch (family) {
	case PrimitiveFamily::eBool:
		return Primitive::eBool;
	case PrimitiveFamily::eInt:
		return Primitive::eInt32;
	case PrimitiveFamily::eUInt:
		return Primitive::eUInt32;
	case PrimitiveFamily::eFloat:
		return Primitive::eFloat;
	default:
		break;
	}

	fatal("unsupported scalar primitive family");
}

Primitive vector_primitive(PrimitiveFamily family, size_t dim)
{
	assertion(dim >= 2 and dim <= 4, "vector dimension out of range");
	switch (family) {
	case PrimitiveFamily::eInt:
		return Primitive(std::to_underlying(Primitive::eIVec2) + (dim - 2));
	case PrimitiveFamily::eUInt:
		return Primitive(std::to_underlying(Primitive::eUVec2) + (dim - 2));
	case PrimitiveFamily::eFloat:
		return Primitive(std::to_underlying(Primitive::eVec2) + (dim - 2));
	default:
		break;
	}

	fatal("unsupported vector primitive family");
}

Primitive matrix_primitive(PrimitiveFamily family, size_t cols, size_t rows)
{
	assertion(cols >= 2 and cols <= 4, "matrix columns out of range");
	assertion(rows >= 2 and rows <= 4, "matrix rows out of range");

	const auto base = [&]() {
		switch (family) {
		case PrimitiveFamily::eInt:
			return Primitive::eIMat2x2;
		case PrimitiveFamily::eUInt:
			return Primitive::eUMat2x2;
		case PrimitiveFamily::eFloat:
			return Primitive::eFMat2x2;
		default:
			break;
		}
		fatal("unsupported matrix primitive family");
	}();

	return Primitive(std::to_underlying(base) + ((cols - 2) * 3 + (rows - 2)));
}

Reference get_or_add_type_ref(const SharedBlockReference &sbr, const Reference &ref);

Reference operation_type_ref(const SharedBlockReference &sbr, const Operation &opn)
{
	switch (opn.code) {
	case OperationCode::eEqual:
	case OperationCode::eGreater:
	case OperationCode::eGreaterEqual:
	case OperationCode::eLess:
	case OperationCode::eLessEqual:
	case OperationCode::eLogicalAnd:
	case OperationCode::eLogicalNot:
	case OperationCode::eLogicalOr:
	case OperationCode::eLogicalXor:
	case OperationCode::eNotEqual:
		return get_or_add_type(sbr, Primitive::eBool);
	default:
		break;
	}

	auto atype = get_or_add_type_ref(sbr, opn.a);
	if (not opn.b)
		return atype;

	auto btype = get_or_add_type_ref(sbr, opn.b);
	auto &alhs = atype->as <Type> ();
	auto &arhs = btype->as <Type> ();

	if (not alhs.is <Primitive> () or not arhs.is <Primitive> ())
		return atype;

	auto lhs = alhs.as <Primitive> ();
	auto rhs = arhs.as <Primitive> ();
	if (primitive_family(lhs) != primitive_family(rhs))
		return atype;

	// Scalar-vector/matrix promotion (applies to all arithmetic ops)
	if (primitive_is_scalar(lhs) and not primitive_is_scalar(rhs))
		return btype;

	if (opn.code != OperationCode::eMultiply)
		return atype;

	if (primitive_is_matrix(lhs) and primitive_is_vector(rhs)) {
		auto cols = primitive_matrix_columns(lhs);
		auto rows = primitive_matrix_rows(lhs);
		auto rdim = primitive_vector_dimension(rhs);
		if (cols == rdim)
			return get_or_add_type(sbr, vector_primitive(primitive_family(lhs), rows));
	}

	if (primitive_is_vector(lhs) and primitive_is_matrix(rhs)) {
		auto ldim = primitive_vector_dimension(lhs);
		auto rows = primitive_matrix_rows(rhs);
		auto cols = primitive_matrix_columns(rhs);
		if (ldim == rows)
			return get_or_add_type(sbr, vector_primitive(primitive_family(rhs), cols));
	}

	if (primitive_is_matrix(lhs) and primitive_is_matrix(rhs)) {
		auto lcols = primitive_matrix_columns(lhs);
		auto lrows = primitive_matrix_rows(lhs);
		auto rcols = primitive_matrix_columns(rhs);
		auto rrows = primitive_matrix_rows(rhs);
		if (lcols == rrows)
			return get_or_add_type(sbr, matrix_primitive(primitive_family(lhs), rcols, lrows));
	}

	return atype;
}

Reference builtin_intrinsic_type_ref(const SharedBlockReference &sbr, const BuiltinIntrinsic &bintr)
{
	auto arg_type = [&](size_t i) {
		assertion(i < bintr.args.size(), "intrinsic argument index out of range");
		return get_or_add_type_ref(sbr, bintr.args[i]);
	};

	switch (bintr.code) {
	case BuiltinIntrinsicCode::eBreak:
	case BuiltinIntrinsicCode::eContinue:
	case BuiltinIntrinsicCode::eDiscard:
	case BuiltinIntrinsicCode::eEmitMeshTasksEXT:
	case BuiltinIntrinsicCode::eSetMeshOutputsEXT:
		fatal("intrinsic {} has no value type", repr(bintr.code));
	case BuiltinIntrinsicCode::eDot:
	case BuiltinIntrinsicCode::eLength:
	case BuiltinIntrinsicCode::eDistance: {
		auto &type = arg_type(0)->as <Type> ();
		assertion(type.is <Primitive> (), "expected Primitive type for dot/length/distance");
		auto family = primitive_family(type.as <Primitive> ());
		return get_or_add_type(sbr, scalar_primitive(family));
	}
	case BuiltinIntrinsicCode::eSample:
	case BuiltinIntrinsicCode::eImageLoad:
	case BuiltinIntrinsicCode::eTexelFetch: {
		auto &type = arg_type(0)->as <Type> ();
		assertion(type.is <Primitive> (), "expected Primitive type for sample/imageLoad/texelFetch");
		auto family = primitive_family(type.as <Primitive> ());
		return get_or_add_type(sbr, vector_primitive(family, 4));
	}
	case BuiltinIntrinsicCode::eSelect:
		return arg_type(1);
	case BuiltinIntrinsicCode::eIntBitsToFloat:
	case BuiltinIntrinsicCode::eUintBitsToFloat:
		return get_or_add_type(sbr, Primitive::eFloat);
	case BuiltinIntrinsicCode::eFloatBitsToInt:
		return get_or_add_type(sbr, Primitive::eInt32);
	case BuiltinIntrinsicCode::eFloatBitsToUint:
		return get_or_add_type(sbr, Primitive::eUInt32);
	case BuiltinIntrinsicCode::eIsNan:
	case BuiltinIntrinsicCode::eIsInf:
		return get_or_add_type(sbr, Primitive::eBool);
	default:
		break;
	}

	return arg_type(0);
}

Reference get_or_add_type_ref(const SharedBlockReference &sbr, const Reference &ref)
{
	vswitch (*ref) {
	vcase(Argument): {
		auto &arg = ref->as <Argument> ();
		return get_or_add_type_ref(sbr, arg.type);
	}
	vcase(BuiltinIntrinsic): {
		auto &bintr = ref->as <BuiltinIntrinsic> ();
		return builtin_intrinsic_type_ref(sbr, bintr);
	}
	vcase(Constant): {
		auto &constant = ref->as <Constant> ();
		return std::visit([&] <typename T> (T x) -> Reference {
			using U = std::remove_cvref_t <T>;
			if constexpr (std::is_same_v <U, bool>)
				return get_or_add_type(sbr, Primitive::eBool);
			else if constexpr (std::is_same_v <U, int32_t>)
				return get_or_add_type(sbr, Primitive::eInt32);
			else if constexpr (std::is_same_v <U, uint32_t>)
				return get_or_add_type(sbr, Primitive::eUInt32);
			else if constexpr (std::is_same_v <U, float>)
				return get_or_add_type(sbr, Primitive::eFloat);
			else
				fatal("unsupported constant type in get_or_add_type_ref");
		}, constant);
	}
	vcase(Construct): {
		auto &ctor = ref->as <Construct> ();
		return get_or_add_type_ref(sbr, ctor.type);
	}
	vcase(Local): {
		auto &local = ref->as <Local> ();
		return get_or_add_type_ref(sbr, local.type);
	}
	vcase(Type): {
		return ref;
	}
	vcase(GlobalResource): {
		auto &grsrc = ref->as <GlobalResource> ();
		auto base = get_or_add_type_ref(sbr, grsrc.type);
		if (grsrc.count)
			return get_or_add_type(sbr, Array { base, *grsrc.count });
		return base;
	}
	vcase(ArrayAccess): {
		auto &aacc = ref->as <ArrayAccess> ();
		auto &type = get_or_add_type_ref(sbr, aacc.value)->as <Type> ();
		assertion(type.is <Array> (), "expected Array type in ArrayAccess");
		return get_or_add_type_ref(sbr, type.as <Array> ().base);
	}
	vcase(FieldAccess): {
		auto &facc = ref->as <FieldAccess> ();
		auto &type = get_or_add_type_ref(sbr, facc.value)->as <Type> ();
		if (type.is <BufferReferenceType> ()) {
			assertion(facc.fidx == 0, "BufferReferenceType field index must be 0");
			auto &brt = type.as <BufferReferenceType> ();
			return get_or_add_type_ref(sbr, brt.element_type);
		}
		assertion(type.is <Struct> (), "expected Struct type in FieldAccess");
		return get_or_add_type_ref(sbr, type.as <Struct>().at(facc.fidx));
	}
	vcase(SystemValue): {
		auto &sv = ref->as <SystemValue> ();
		switch (sv) {
		case SystemValue::eClipPosition:
			return get_or_add_type(sbr, Primitive::eVec4);
		case SystemValue::eGlobalInvocationID:
		case SystemValue::eLocalInvocationID:
		case SystemValue::eWorkGroupID:
		case SystemValue::eLaunchID:
		case SystemValue::eLaunchSize:
			return get_or_add_type(sbr, Primitive::eUVec3);
		case SystemValue::eInstanceIndex:
		case SystemValue::eVertexIndex:
		case SystemValue::ePrimitiveID:
		case SystemValue::eInstanceCustomIndex:
			return get_or_add_type(sbr, Primitive::eUInt32);
		case SystemValue::eHitT:
			return get_or_add_type(sbr, Primitive::eFloat);
		case SystemValue::eWorldRayOrigin:
		case SystemValue::eWorldRayDirection:
			return get_or_add_type(sbr, Primitive::eVec3);
		case SystemValue::eTaskPayload:
			return get_or_add_type_ref(sbr, sbr->task_payload_type.value());
		case SystemValue::eHitAttribute:
			return get_or_add_type_ref(sbr, sbr->hit_attribute_type.value());
		case SystemValue::eMeshVertices: {
			auto base = get_or_add_type(sbr, Primitive::eVec4);
			return get_or_add_type(sbr, Array { base, sbr->mesh_max_vertices.value() });
		}
		case SystemValue::ePrimitiveTriangleIndices: {
			auto base = get_or_add_type(sbr, Primitive::eUVec3);
			return get_or_add_type(sbr, Array { base, sbr->mesh_max_primitives.value() });
		}
		default:
			break;
		}
		break;
	}
	vcase(Operation): {
		auto &opn = ref->as <Operation> ();
		return operation_type_ref(sbr, opn);
	}
	vcase(Return): {
		auto &ret = ref->as <Return> ();
		return get_or_add_type_ref(sbr, ret.type);
	}
	vcase(StageInput): {
		auto &sin = ref->as <StageInput> ();
		return get_or_add_type_ref(sbr, sin.type);
	}
	vcase(StageOutput): {
		auto &sout = ref->as <StageOutput> ();
		return get_or_add_type_ref(sbr, sout.type);
	}
	vcase(Swizzle): {
		auto &swz = ref->as <Swizzle> ();
		auto type = get_or_add_type_ref(sbr, swz.value)->as <Type> ();
		assertion(type.is <Primitive> (), "expected Primitive type for swizzle");
		auto prim = type.as <Primitive> ();
		return get_or_add_type(sbr, swizzle_type(prim, swz.code));
	}
	default:
		break;
	}

	fatal("unhandled instruction in get_or_add_type_ref: {}", ref->repr());
}

} // namespace rcgp
