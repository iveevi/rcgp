#include <rcgp.hpp>

using namespace rcgp;

// Plain POD fields should pack tightly with natural alignments (scalar layout keeps C++ rules).
struct A {
	i32 a;
	u32 b;
	f32 c;

	$reflection(a, b, c);
};

using MA = layouts::apply_t <A, layouts::scalar>;
static_assert(alignof(MA) == 4);
static_assert(sizeof(MA) == 12);
static_assert(offsetof(MA, a) == 0);
static_assert(offsetof(MA, b) == 4);
static_assert(offsetof(MA, c) == 8);

// vec2 stays at scalar alignment; tail scalar should follow without 16-byte padding.
struct Vec2Tail {
	vector <float, 2> uv;
	f32 weight;

	$reflection(uv, weight);
};

using MVec2Tail = layouts::apply_t <Vec2Tail, layouts::scalar>;
static_assert(alignof(MVec2Tail) == 4);
static_assert(sizeof(MVec2Tail) == 12);
static_assert(offsetof(MVec2Tail, uv) == 0);
static_assert(offsetof(MVec2Tail, weight) == 8);

// vec3 also keeps scalar alignment (4); only minimal padding is inserted.
struct Padding {
	f32 x;
	vector <float, 3> v;

	$reflection(x, v);
};

using MPadding = layouts::apply_t <Padding, layouts::scalar>;
static_assert(alignof(MPadding) == 4);
static_assert(sizeof(MPadding) == 16);
static_assert(offsetof(MPadding, x) == 0);
static_assert(offsetof(MPadding, v) == 4);

// Nested aggregate should simply stack its scalar-aligned members.
struct Nested {
	vector <float, 3> pos;
	f32 temp;

	$reflection(pos, temp);
};

using MNested = layouts::apply_t <Nested, layouts::scalar>;
static_assert(alignof(MNested) == 4);
static_assert(sizeof(MNested) == 16);
static_assert(offsetof(MNested, pos) == 0);
static_assert(offsetof(MNested, temp) == 12);

// Outer aggregate mixing scalar and nested structs; no 16-byte rounding expected.
struct Outer {
	i32 id;
	Nested inner;
	u32 flags;

	$reflection(id, inner, flags);
};

using MOuter = layouts::apply_t <Outer, layouts::scalar>;
static_assert(alignof(MOuter) == 4);
static_assert(sizeof(MOuter) == 24);
static_assert(offsetof(MOuter, id) == 0);
static_assert(offsetof(MOuter, inner) == 4);
static_assert(offsetof(MOuter, flags) == 20);

// Statically sized array of vec3 retains 4-byte alignment per element.
struct VectorArray {
	array <vector <float, 3>, 2> elements;

	$reflection(elements);
};

using MVectorArray = layouts::apply_t <VectorArray, layouts::scalar>;
static_assert(alignof(MVectorArray) == 4);
static_assert(sizeof(MVectorArray) == 24);
static_assert(alignof(decltype(MVectorArray::elements)::value_type) == 4);
static_assert(sizeof(decltype(MVectorArray::elements)::value_type) == 12);
static_assert(offsetof(MVectorArray, elements) == 0);

// Runtime array header plus dynamically sized vec3 payload; alignment stays at scalar granularity.
struct RuntimeBuffer {
	u32 count;
	array <vector <float, 3>> values;

	$reflection(count, values);
};

using MRuntimeBuffer = layouts::apply_t <RuntimeBuffer, layouts::scalar>;
static_assert(alignof(MRuntimeBuffer) == 8); // std::vector base typically 8 on 64-bit.
static_assert(offsetof(MRuntimeBuffer, count) == 0);
static_assert(offsetof(MRuntimeBuffer, values) == 8);
static_assert(alignof(typename decltype(MRuntimeBuffer::values)::value_type) == 4);
