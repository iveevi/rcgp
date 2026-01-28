#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

#include "dsl/aliases.hpp"
#include "dsl/array.hpp"
#include "dsl/generators.hpp"
#include "dsl/instruction_block.hpp"
#include "dsl/jems.hpp"
#include "meta/inject_reference.hpp"
#include "meta/reconstruct_type.hpp"
#include "util/logging.hpp"

using namespace rcgp;

struct RecordingPair {
	f32 x;
	i32 y;
	$reflection(x, y);
};

static SharedBlockReference trace_block(const std::function <void ()> &fn)
{
	Tracer::singleton.type_cache.clear();
	auto sbr = std::make_shared <Block> ();
	{
		jems::scope scope(sbr);
		fn();
	}
	return sbr;
}

#include "recording_match.cpp"

void test_recording_scalar_constant()
{
	auto sbr = trace_block([] {
		i32 a = 3;
		(void)a;
	});
	auto expected = trace_block([] {
		auto type = jems::type(int32_t());
		auto local = jems::local(type);
		auto value = jems::constant(int32_t(3));
		jems::store(local, value);
	});
	assert_block_sequence("scalar constant", sbr, expected);
	info("recording_scalar_constant:\n%s", generate_assembly(sbr).c_str());
}

void test_recording_binary_op_add()
{
	auto sbr = trace_block([&] {
		i32 a = 1;
		i32 b = 2;
		i32 c = a + b;
		(void)c;
	});
	auto expected = trace_block([] {
		auto type = jems::type(int32_t());
		auto a = jems::local(type);
		auto one = jems::constant(int32_t(1));
		jems::store(a, one);
		auto b = jems::local(type);
		auto two = jems::constant(int32_t(2));
		jems::store(b, two);
		jems::operation(OperationCode::eAdd, a, b);
	});
	assert_block_sequence("binary add", sbr, expected);
	info("recording_binary_op_add:\n%s", generate_assembly(sbr).c_str());
}

void test_recording_unary_op_negate()
{
	auto sbr = trace_block([&] {
		i32 a = 1;
		i32 b = -a;
		(void)b;
	});
	auto expected = trace_block([] {
		auto type = jems::type(int32_t());
		auto a = jems::local(type);
		auto one = jems::constant(int32_t(1));
		jems::store(a, one);
		auto neg = jems::local(type);
		auto minus_one = jems::constant(int32_t(-1));
		jems::store(neg, minus_one);
		jems::operation(OperationCode::eMultiply, neg, a);
	});
	assert_block_sequence("unary negate", sbr, expected);
	info("recording_unary_op_negate:\n%s", generate_assembly(sbr).c_str());
}

void test_recording_assignment_store()
{
	auto sbr = trace_block([&] {
		i32 a;
		i32 b = 7;
		a = b;
	});
	auto expected = trace_block([] {
		auto type = jems::type(int32_t());
		auto a = jems::local(type);
		auto b = jems::local(type);
		auto seven = jems::constant(int32_t(7));
		jems::store(b, seven);
		jems::store(a, b);
	});
	assert_block_sequence("assignment store", sbr, expected);
	info("recording_assignment_store:\n%s", generate_assembly(sbr).c_str());
}

void test_recording_increment_decrement()
{
	auto sbr = trace_block([&] {
		i32 a = 1;
		++a;
		a++;
		--a;
		a--;
	});
	auto expected = trace_block([] {
		auto type = jems::type(int32_t());
		auto a = jems::local(type);
		auto one = jems::constant(int32_t(1));
		jems::store(a, one);

		auto inc1 = jems::local(type);
		auto incv1 = jems::constant(int32_t(1));
		jems::store(inc1, incv1);
		auto add1 = jems::operation(OperationCode::eAdd, a, inc1);
		jems::store(a, add1);

		auto inc2 = jems::local(type);
		auto incv2 = jems::constant(int32_t(1));
		jems::store(inc2, incv2);
		auto add2 = jems::operation(OperationCode::eAdd, a, inc2);
		jems::store(a, add2);

		auto dec1 = jems::local(type);
		auto decv1 = jems::constant(int32_t(1));
		jems::store(dec1, decv1);
		auto sub1 = jems::operation(OperationCode::eSubtract, a, dec1);
		jems::store(a, sub1);

		auto dec2 = jems::local(type);
		auto decv2 = jems::constant(int32_t(1));
		jems::store(dec2, decv2);
		auto sub2 = jems::operation(OperationCode::eSubtract, a, dec2);
		jems::store(a, sub2);
	});
	assert_block_sequence("increment/decrement", sbr, expected);
	info("recording_increment_decrement:\n%s", generate_assembly(sbr).c_str());
}

void test_recording_type_caching()
{
	auto sbr = trace_block([] {
		i32 a;
		i32 b;
		(void)a;
		(void)b;
	});
	auto expected = trace_block([] {
		auto type = jems::type(int32_t());
		auto a = jems::local(type);
		auto b = jems::local(type);
		(void)a;
		(void)b;
	});
	assert_block_sequence("type caching", sbr, expected);
	info("recording_type_caching:\n%s", generate_assembly(sbr).c_str());
}

void test_recording_construct_vec3()
{
	auto sbr = trace_block([] {
		f32 x = 1.0f;
		f32 y = 2.0f;
		f32 z = 3.0f;
		vec3 v(x, y, z);
		(void)v;
	});
	auto expected = trace_block([] {
		auto f32_type = jems::type(float());
		auto x = jems::local(f32_type);
		auto xval = jems::constant(1.0f);
		jems::store(x, xval);
		auto y = jems::local(f32_type);
		auto yval = jems::constant(2.0f);
		jems::store(y, yval);
		auto z = jems::local(f32_type);
		auto zval = jems::constant(3.0f);
		jems::store(z, zval);

		auto vec3_type = jems::type(VectorType <float, 3> ());
		auto ctor = jems::construct(vec3_type, x, y, z);
		auto v = jems::local(vec3_type);
		jems::store(v, ctor);
	});
	assert_block_sequence("construct vec3", sbr, expected);
	info("recording_construct_vec3:\n%s", generate_assembly(sbr).c_str());
}

void test_recording_swizzle_xyz()
{
	auto sbr = trace_block([] {
		vec4 v;
		vec3 xyz = v.xyz;
		(void)xyz;
	});
	auto expected = trace_block([] {
		auto vec4_type = jems::type(VectorType <float, 4> ());
		auto v = jems::local(vec4_type);
		jems::swizzle(SwizzleCode::eXYZ, v);
	});
	assert_block_sequence("swizzle xyz", sbr, expected);
	info("recording_swizzle_xyz:\n%s", generate_assembly(sbr).c_str());
}

void test_recording_field_access()
{
	auto sbr = trace_block([] {
		RecordingPair p;
		auto type = reconstruct_type <RecordingPair> ();
		inject_reference(p, jems::local(type));
		auto x = p.x;
		auto y = p.y;
		(void)x;
		(void)y;
	});
	auto expected = trace_block([] {
		auto f32_type = jems::type(float());
		auto x = jems::local(f32_type);
		auto i32_type = jems::type(int32_t());
		auto y = jems::local(i32_type);
		(void)x;
		(void)y;

		AggregateType aggregate;
		aggregate.name = "RecordingPair";
		aggregate.emplace_back(f32_type);
		aggregate.emplace_back(i32_type);
		auto pair_type = jems::type(aggregate);
		auto pair = jems::local(pair_type);
		jems::field_access(pair, 0);
		jems::field_access(pair, 1);
	});
	assert_block_sequence("field access", sbr, expected);
	info("recording_field_access:\n%s", generate_assembly(sbr).c_str());
}

void test_recording_array_access()
{
	auto sbr = trace_block([] {
		array <i32, 4> values;
		auto type = reconstruct_type <array <i32, 4>> ();
		inject_reference(values, jems::local(type));
		i32 v = values[2];
		(void)v;
	});
	auto expected = trace_block([] {
		auto i32_type = jems::type(int32_t());
		auto arr_type = jems::type(ArrayType(i32_type, 4));
		auto arr = jems::local(arr_type);
		auto v = jems::local(i32_type);
		(void)v;
		auto idx = jems::local(i32_type);
		auto idx_val = jems::constant(int32_t(2));
		jems::store(idx, idx_val);
		jems::array_access(arr, idx);
	});
	assert_block_sequence("array access", sbr, expected);
	info("recording_array_access:\n%s", generate_assembly(sbr).c_str());
}
