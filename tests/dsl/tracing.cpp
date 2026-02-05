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
	    stage outputs: { Smooth $0, Flat $1 }
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
		$contracts((p, fwd::position)),
		ClipPosition cpos
	) -> float3
	{
		cpos = float4(p, 1);
		return p;
	};

	assert_assembly_match(vs, R"(
	Block {
	  Context {
	    stage: Vertex,
	    stage inputs: { Smooth $0 }
	    stage outputs: { Smooth $0 }
	  }
	  $0 = Vec3
	  $1 = Local $0
	  $2 = StageInput 0: Smooth $0
	  $3 = Float
	  $4 = Local $3
	  $5 = 1
	  Store $4 $5
	  $6 = Vec4
	  $7 = New $6($2, $4)
	  $8 = Local $6
	  Store $8 $7
	  $9 = SV: ClipPosition
	  Store $9 $8
	  $10 = StageOutput 0: Smooth $0
	  Store $10 $2
	}
	)");
};

add_test(vs_multiple_io)
{
	auto vs = $shader(vertex)(
		$contracts(
			(pos, fwd::position),
			(nrm, fwd::normal),
			(uv, fwd::uv)
		)
	) {
		return std::tuple {
			Smooth <float3> { pos },
			Smooth <float3> { nrm },
			Smooth <float2> { uv },
		};
	};
	
	assert_assembly_match(vs, R"(
	Block {
	  Context {
	    stage: Vertex,
	    stage inputs: { Smooth $0, Smooth $0, Smooth $1 }
	    stage outputs: { Smooth $0, Smooth $0, Smooth $1 }
	  }
	  $1 = Vec2
	  $2 = Local $1
	  $0 = Vec3
	  $3 = Local $0
	  $4 = Local $0
	  $5 = StageInput 0: Smooth $0
	  $6 = StageInput 1: Smooth $0
	  $7 = StageInput 2: Smooth $1
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
		$contracts(
			(view, fwd::view),
			(pos, fwd::position)
		),
		ClipPosition cpos
	) -> float3
	{
		float4 wpos = view.model * float4(pos, 1);
		cpos = view.proj * view.view * wpos;
		return float3(wpos);
	};

	assert_assembly_match(vs, R"(
	Block {
	  Context {
	    stage: Vertex,
	    stage inputs: { Smooth $0 }
	    stage outputs: { Smooth $0 }
	    resources: { $1 },
	  }
	  $0 = Vec3
	  $2 = Local $0
	  $3 = FMat4x4
	  $4 = Local $3
	  $5 = Local $3
	  $6 = Local $3
	  $7 = fwd::View { model: $3, view: $3, proj: $3 }
	  $1 = PushConstant +4294967295: Std430 $7
	  $8 = $1.model
	  $9 = $1.view
	  $10 = $1.proj
	  $11 = StageInput 0: Smooth $0
	  $12 = Float
	  $13 = Local $12
	  $14 = 1
	  Store $13 $14
	  $15 = Vec4
	  $16 = New $15($11, $13)
	  $17 = Local $15
	  Store $17 $16
	  $18 = Multiply $8 $17
	  $19 = Local $15
	  Store $19 $18
	  $20 = Multiply $10 $9
	  $21 = Local $3
	  Store $21 $20
	  $22 = Multiply $21 $19
	  $23 = Local $15
	  Store $23 $22
	  $24 = SV: ClipPosition
	  Store $24 $23
	  $25 = New $0($19)
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
	  $1 = UInt32
	  $4 = Local $1
	  $0 = Float
	  $5 = Local $0
	  $6 = Argument 0: $0
	  $7 = Argument 1: $1
	  $2 = Vec3
	  $8 = New $2($6, $6, $6)
	  $9 = Local $2
	  Store $9 $8
	  $10 = Local $1
	  $11 = 13
	  Store $10 $11
	  $3 = UVec2
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
	  $0 = Float
	  $2 = Local $0
	  $3 = Argument 0: $0
	  $4 = Local $0
	  $5 = 0
	  Store $4 $5
	  $6 = Vec3
	  $7 = New $6($4, $4, $4)
	  $8 = Local $6
	  Store $8 $7
	  $9 = Local $0
	  $10 = 1
	  Store $9 $10
	  $11 = Local $0
	  $12 = 1
	  Store $11 $12
	  $13 = New $6($11, $3, $9)
	  $14 = Local $6
	  Store $14 $13
	  $15 = Normalize($14)
	  $16 = Local $6
	  Store $16 $15
	  $1 = Ray { origin: $6, direction: $6 }
	  $17 = Return 0: $1
	  $18 = New $1($8, $16)
	  Store $17 $18
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
	    stage outputs: { Smooth $0, Smooth $0, Smooth $1, Smooth $0, Smooth $0 }
	  }
	  $2 = Float
	  $3 = Local $2
	  $4 = 1
	  Store $3 $4
	  $0 = Vec3
	  $5 = Local $0
	  sr1($3, $5)
	  $6 = Local $0
	  $7 = UInt32
	  $8 = Local $7
	  $9 = 2
	  Store $8 $9
	  $10 = Local $2
	  $11 = 1
	  Store $10 $11
	  $12 = Local $0
	  $1 = UVec2
	  $13 = Local $1
	  sr2($10, $8, $12, $13)
	  $14 = Local $1
	  $15 = Local $0
	  $16 = Local $2
	  $17 = 2
	  Store $16 $17
	  $18 = Ray { origin: $0, direction: $0 }
	  $19 = Local $18
	  sr3($16, $19)
	  $20 = Local $0
	  $21 = Local $0
	  $22 = $19.origin
	  $23 = $19.direction
	  $24 = StageOutput 0: Smooth $0
	  Store $24 $5
	  $25 = StageOutput 1: Smooth $0
	  Store $25 $13
	  $26 = StageOutput 2: Smooth $1
	  Store $26 $12
	  $27 = StageOutput 3: Smooth $0
	  Store $27 $22
	  $28 = StageOutput 4: Smooth $0
	  Store $28 $23
	}
	)");
};
