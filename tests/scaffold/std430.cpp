#include <rcgp.hpp>

using namespace rcgp;

// Plain POD fields should pack tightly with natural alignments.
struct A {
	i32 a;
	u32 b;
	f32 c;

	$reflection(a, b, c);
};

using MA = layouts::apply_t <A, layouts::std430>;
static_assert(sizeof(MA) == 12);
static_assert(offsetof(MA, a) == 0);
static_assert(offsetof(MA, b) == 4);
static_assert(offsetof(MA, c) == 8);
static_assert(std::constructible_from <decltype(MA::a), int32_t>);
static_assert(std::constructible_from <decltype(MA::b), uint32_t>);
static_assert(std::constructible_from <decltype(MA::c), float>);

// vec2 (8-byte alignment) followed by scalar should align to 8, pad to 16 bytes total.
struct Vec2Tail {
	vector <float, 2> uv;
	f32 weight;

	$reflection(uv, weight);
};

using MVec2Tail = layouts::apply_t <Vec2Tail, layouts::std430>;
static_assert(alignof(MVec2Tail) == 8);
static_assert(sizeof(MVec2Tail) == 16);
static_assert(offsetof(MVec2Tail, uv) == 0);
static_assert(offsetof(MVec2Tail, weight) == 8);

// vec3 requires 16-byte base alignment under std430; ensure padding is inserted before it.
struct Padding {
	f32 x;
	vector <float, 3> v;

	$reflection(x, v);
};

using MPadding = layouts::apply_t <Padding, layouts::std430>;
static_assert(alignof(MPadding) == 16);
static_assert(sizeof(MPadding) == 32);
static_assert(offsetof(MPadding, x) == 0);
static_assert(offsetof(MPadding, v) == 16);
static_assert(alignof(decltype(MPadding::v)) == 4);

// Internal padding inside an aggregate with vec3 plus scalar.
struct Nested {
	vector <float, 3> pos;
	f32 temp;

	$reflection(pos, temp);
};

using MNested = layouts::apply_t <Nested, layouts::std430>;
static_assert(alignof(MNested) == 16);
static_assert(sizeof(MNested) == 16);
static_assert(offsetof(MNested, pos) == 0);
static_assert(offsetof(MNested, temp) == 12);

// Nested aggregate should respect inner alignment and pad surrounding fields.
struct Outer {
	i32 id;
	Nested inner;
	u32 flags;

	$reflection(id, inner, flags);
};

using MOuter = layouts::apply_t <Outer, layouts::std430>;
static_assert(alignof(MOuter) == 16);
static_assert(sizeof(MOuter) == 48);
static_assert(offsetof(MOuter, id) == 0);
static_assert(offsetof(MOuter, inner) == 16);
static_assert(offsetof(MOuter, flags) == 32);

// Statically sized array of vec3 must align each element to 16 bytes.
struct VectorArray {
	array <vector <float, 3>, 2> elements;

	$reflection(elements);
};

using MVectorArray = layouts::apply_t <VectorArray, layouts::std430>;
static_assert(alignof(MVectorArray) == 16);
static_assert(sizeof(MVectorArray) == 32);
static_assert(alignof(decltype(MVectorArray::elements)::value_type) == 16);
static_assert(sizeof(decltype(MVectorArray::elements)::value_type) == 16);
static_assert(offsetof(MVectorArray, elements) == 0);

// Runtime array should place header then align unsized vec3 elements to 16-byte boundary.
struct RuntimeBuffer {
	u32 count;
	array <vector <float, 3>> values;

	$reflection(count, values);
};

using MRuntimeBuffer = layouts::apply_t <RuntimeBuffer, layouts::std430>;
static_assert(alignof(MRuntimeBuffer) == 16);
static_assert(offsetof(MRuntimeBuffer, count) == 0);
static_assert(offsetof(MRuntimeBuffer, values) == 16);
static_assert(alignof(typename decltype(MRuntimeBuffer::values)::value_type) == 16);
