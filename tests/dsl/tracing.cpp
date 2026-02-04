#include "common.hpp"
#include "common_resources.hpp"

#define SUITE "tracing"

// Tracing:
// Testing that user-authored shader modules and subroutines are correctly
// trascribed into instructions. This primarily tests for argument injection,
// return handling, and subroutine invocation.
// TODO: verbose_tracing and normal tracing

add_test(vs_empty)
{
	auto vs = $shader(vertex)() {};

	assert_assembly_match(vs, R"(
	Block {
	  Context {
	    stage: Vertex,
	  }
	}
	)");
};

add_test(vs_clip)
{
	auto vs = $shader(vertex)(ClipPosition clip)
	{
		clip = float4(1);
	};

	assert_assembly_match(vs, R"(
	Block {
	  Context {
	    stage: Vertex,
	  }
	  $0 = Float
	  $1 = Local $0
	  $2 = 1
	  Store $1 $2
	  $3 = Vec4
	  $4 = New $3($1, $1, $1, $1)
	  $5 = Local $3
	  Store $5 $4
	  $6 = SV: ClipPosition
	  Store $6 $5
	}
	)");
};

add_test(vs_louts)
{
	auto vs = $shader(vertex)()
	{
		return std::tuple {
			Smooth { float3(1.0) },
			Flat { uint2(1, 4) },
		};
	};

	assert_assembly_match(vs, R"(
	Block {
	  Context {
	    stage: Vertex,
	    stage inputs: { Smooth $0, Flat $1 }
	  }
	  $2 = Float
	  $3 = Local $2
	  $4 = 1
	  Store $3 $4
	  $0 = Vec3
	  $5 = New $0($3, $3, $3)
	  $6 = Local $0
	  Store $6 $5
	  $7 = StageOutput 0: Smooth $0
	  Store $7 $6
	  $8 = UInt32
	  $9 = Local $8
	  $10 = 4
	  Store $9 $10
	  $11 = Local $8
	  $12 = 1
	  Store $11 $12
	  $1 = UVec2
	  $13 = New $1($11, $9)
	  $14 = Local $1
	  Store $14 $13
	  $15 = StageOutput 1: Flat $1
	  Store $15 $14
	}
	)");
};

add_test(vs_stream)
{
	auto vs = $shader(vertex)(
		$contracts(position),
		ClipPosition cpos
	) -> float3
	{
		cpos = float4(position, 1);
		return position;
	};

	assert_assembly_match(vs, R"(
	Block {
	  Context {
	    stage: Vertex,
	    stage inputs: { $0 }
	    stage inputs: { Smooth $0 }
	  }
	  $1 = Vec4
	  $2 = Float
	  $0 = Vec3
	  $3 = Local $0
	  $4 = StageInput 0: $0
	  $5 = Local $2
	  $6 = 1
	  Store $5 $6
	  $7 = New $1($4, $5)
	  $8 = Local $1
	  Store $8 $7
	  $9 = SV: ClipPosition
	  Store $9 $8
	  $10 = StageOutput 0: Smooth $0
	  Store $10 $4
	}
	)");
};

add_test(vs_multiple_io)
{
	auto vs = $shader(vertex)($contracts(position, normal, uv)) {
		return std::tuple {
			Smooth <float3> { position },
			Smooth <float3> { normal },
			Smooth <float2> { uv },
		};
	};
	
	assert_assembly_match(vs, R"(
	Block {
	  Context {
	    stage: Vertex,
	    stage inputs: { $0, $0, $1 }
	    stage inputs: { Smooth $0, Smooth $0, Smooth $1 }
	  }
	  $0 = Vec3
	  $1 = Vec2
	  $2 = Local $1
	  $3 = Local $0
	  $4 = Local $0
	  $5 = StageInput 0: $0
	  $6 = StageInput 1: $0
	  $7 = StageInput 2: $1
	  $8 = StageOutput 0: Smooth $0
	  Store $8 $5
	  $9 = StageOutput 1: Smooth $0
	  Store $9 $6
	  $10 = StageOutput 2: Smooth $1
	  Store $10 $7
	}
	)");
};

add_test(vs_push_constant)
{
	auto vs = $shader(vertex)(
		$contracts(view, position),
		ClipPosition cpos
	) -> float3
	{
		float4 wpos = view.model * float4(position, 1);
		cpos = view.proj * view.view * wpos;
		return float3(wpos);
	};

	assert_assembly_match(vs, R"(
	Block {
	  Context {
	    stage: Vertex,
	    stage inputs: { $0 }
	    stage inputs: { Smooth $0 }
	    resources: { $1 },
	  }
	  $2 = Vec4
	  $3 = Float
	  $0 = Vec3
	  $4 = Local $0
	  $5 = FMat4x4
	  $6 = Local $5
	  $7 = Local $5
	  $8 = Local $5
	  $9 = Local $5
	  $10 = Local $5
	  $11 = Local $5
	  $12 = View { f0: $5, f1: $5, f2: $5 }
	  $1 = PushConstant +4294967295: Std430 $12
	  $13 = $1.f0
	  $14 = $1.f1
	  $15 = $1.f2
	  $16 = StageInput 0: $0
	  $17 = Local $3
	  $18 = 1
	  Store $17 $18
	  $19 = New $2($16, $17)
	  $20 = Local $2
	  Store $20 $19
	  $21 = Multiply $13 $20
	  $22 = Multiply $15 $14
	  $23 = Multiply $22 $21
	  $24 = SV: ClipPosition
	  Store $24 $23
	  $25 = New $0($21)
	  $26 = Local $0
	  Store $26 $25
	  $27 = StageOutput 0: Smooth $0
	  Store $27 $26
	}
	)");
};

add_test(sr_return_primitives)
{
	auto sr = $subroutine(sr)(f32 x, u32 y) {
		return std::tuple { float3(x), uint2(y, 13) };
	};
	
	assert_assembly_match(sr, R"(
	Block {
	  Context {
	    stage: Subroutine,
	    name: sr,
	    arguments: { $0, $1 },
	    returns: { $2, $3 },
	  }
	  $3 = UVec2
	  $2 = Vec3
	  $0 = Float
	  $1 = UInt32
	  $4 = Local $1
	  $5 = Local $0
	  $6 = Argument 0: $0
	  $7 = Argument 1: $1
	  $8 = New $2($6, $6, $6)
	  $9 = Local $2
	  Store $9 $8
	  $10 = Local $1
	  $11 = 13
	  Store $10 $11
	  $12 = New $3($7, $10)
	  $13 = Local $3
	  Store $13 $12
	  $14 = Return 0: $2
	  Store $14 $9
	  $15 = Return 1: $3
	  Store $15 $13
	}
	)");
};

add_test(sr_return_aggregate)
{
	auto sr = $subroutine(sr)(f32 z) {
		return Ray {
			float3(0),
			normalize(float3(1, z, 1)),
		};
	};
	
	assert_assembly_match(sr, R"(
	Block {
	  Context {
	    stage: Subroutine,
	    name: sr,
	    arguments: { $0 },
	    returns: { $1 },
	  }
	  $2 = Vec3
	  $0 = Float
	  $3 = Local $0
	  $4 = Argument 0: $0
	  $5 = Local $0
	  $6 = 0
	  Store $5 $6
	  $7 = New $2($5, $5, $5)
	  $8 = Local $2
	  Store $8 $7
	  $9 = Local $0
	  $10 = 1
	  Store $9 $10
	  $11 = Local $0
	  $12 = 1
	  Store $11 $12
	  $13 = New $2($11, $4, $9)
	  $14 = Local $2
	  Store $14 $13
	  $15 = Normalize($14)
	  $1 = Ray { f0: $2, f1: $2 }
	  $16 = Return 0: $1
	  $17 = New $1($8, $15)
	  Store $16 $17
	}
	)");
};

add_test(sr_invocation)
{
	auto sr1 = $subroutine(sr1)(f32 x) {
		return float3(x);
	};
	
	auto sr2 = $subroutine(sr2)(f32 x, u32 y) {
		return std::tuple { float3(x), uint2(y, 13) };
	};
	
	auto sr3 = $subroutine(sr3)(f32 z) {
		return Ray {
			float3(0),
			normalize(float3(1, z, 1)),
		};
	};

	auto vs = $shader(vertex, &)() {
		auto r1 = $use(sr1)(1);
		auto [r2a, r2b] = $use(sr2)(1.0f, 2);
		auto r3 = $use(sr3)(2.0f);
		return std::tuple { r1, r2a, r2b, r3.origin, r3.direction };
	};

	assert_assembly_match(vs, R"(
	Block {
	  Context {
	    stage: Vertex,
	    stage inputs: { Smooth $0, Smooth $0, Smooth $1, Smooth $0, Smooth $0 }
	  }
	  $2 = Ray { f0: $0, f1: $0 }
	  $1 = UVec2
	  $3 = UInt32
	  $0 = Vec3
	  $4 = Float
	  $5 = Local $4
	  $6 = 1
	  Store $5 $6
	  $7 = Local $0
	  sr1($5, $7)
	  $8 = Local $0
	  $9 = Local $3
	  $10 = 2
	  Store $9 $10
	  $11 = Local $4
	  $12 = 1
	  Store $11 $12
	  $13 = Local $0
	  $14 = Local $1
	  sr2($11, $9, $13, $14)
	  $15 = Local $1
	  $16 = Local $0
	  $17 = Local $4
	  $18 = 2
	  Store $17 $18
	  $19 = Local $2
	  sr3($17, $19)
	  $20 = Local $0
	  $21 = Local $0
	  $22 = $19.f0
	  $23 = $19.f1
	  $24 = StageOutput 0: Smooth $0
	  Store $24 $7
	  $25 = StageOutput 1: Smooth $0
	  Store $25 $14
	  $26 = StageOutput 2: Smooth $1
	  Store $26 $13
	  $27 = StageOutput 3: Smooth $0
	  Store $27 $22
	  $28 = StageOutput 4: Smooth $0
	  Store $28 $23
	}
	)");
};
