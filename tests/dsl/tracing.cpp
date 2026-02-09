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
	  $1 = StageInput 0: Smooth $0
	  $2 = Float
	  $3 = Local $2
	  $4 = 1
	  Store $3 $4
	  $5 = Vec4
	  $6 = New $5($1, $3)
	  $7 = Local $5
	  Store $7 $6
	  $8 = SV: ClipPosition
	  Store $8 $7
	  $9 = StageOutput 0: Smooth $0
	  Store $9 $1
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
	  $0 = Vec3
	  $2 = StageInput 0: Smooth $0
	  $3 = StageInput 1: Smooth $0
	  $1 = Vec2
	  $4 = StageInput 2: Smooth $1
	  $5 = StageOutput 0: Smooth $0
	  Store $5 $2
	  $6 = StageOutput 1: Smooth $0
	  Store $6 $3
	  $7 = StageOutput 2: Smooth $1
	  Store $7 $4
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
	  $2 = FMat4x4
	  $3 = fwd::View { model: $2, view: $2, proj: $2 }
	  $1 = PushConstant +4294967295: Std430 $3
	  $4 = $1.model
	  $5 = $1.view
	  $6 = $1.proj
	  $0 = Vec3
	  $7 = StageInput 0: Smooth $0
	  $8 = Float
	  $9 = Local $8
	  $10 = 1
	  Store $9 $10
	  $11 = Vec4
	  $12 = New $11($7, $9)
	  $13 = Local $11
	  Store $13 $12
	  $14 = Multiply $4 $13
	  $15 = Local $11
	  Store $15 $14
	  $16 = Multiply $6 $5
	  $17 = Local $2
	  Store $17 $16
	  $18 = Multiply $17 $15
	  $19 = Local $11
	  Store $19 $18
	  $20 = SV: ClipPosition
	  Store $20 $19
	  $21 = New $0($15)
	  $22 = Local $0
	  Store $22 $21
	  $23 = StageOutput 0: Smooth $0
	  Store $23 $22
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
	  $0 = Float
	  $4 = Argument 0: $0
	  $1 = UInt32
	  $5 = Argument 1: $1
	  $2 = Vec3
	  $6 = New $2($4, $4, $4)
	  $7 = Local $2
	  Store $7 $6
	  $8 = Local $1
	  $9 = 13
	  Store $8 $9
	  $3 = UVec2
	  $10 = New $3($5, $8)
	  $11 = Local $3
	  Store $11 $10
	  $12 = Return 0: $2
	  Store $12 $7
	  $13 = Return 1: $3
	  Store $13 $11
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
	  $2 = Argument 0: $0
	  $3 = Local $0
	  $4 = 0
	  Store $3 $4
	  $5 = Vec3
	  $6 = New $5($3, $3, $3)
	  $7 = Local $5
	  Store $7 $6
	  $8 = Local $0
	  $9 = 1
	  Store $8 $9
	  $10 = Local $0
	  $11 = 1
	  Store $10 $11
	  $12 = New $5($10, $2, $8)
	  $13 = Local $5
	  Store $13 $12
	  $14 = Normalize($13)
	  $15 = Local $5
	  Store $15 $14
	  $1 = Ray { origin: $5, direction: $5 }
	  $16 = Return 0: $1
	  $17 = New $1($7, $15)
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
	    stage outputs: { Smooth $0, Smooth $0, Smooth $1, Smooth $0, Smooth $0 }
	  }
	  $2 = Float
	  $3 = Local $2
	  $4 = 1
	  Store $3 $4
	  $0 = Vec3
	  $5 = Local $0
	  sr1($3, $5)
	  $6 = UInt32
	  $7 = Local $6
	  $8 = 2
	  Store $7 $8
	  $9 = Local $2
	  $10 = 1
	  Store $9 $10
	  $11 = Local $0
	  $1 = UVec2
	  $12 = Local $1
	  sr2($9, $7, $11, $12)
	  $13 = Local $2
	  $14 = 2
	  Store $13 $14
	  $15 = Ray { origin: $0, direction: $0 }
	  $16 = Local $15
	  sr3($13, $16)
	  $17 = $16.origin
	  $18 = $16.direction
	  $19 = StageOutput 0: Smooth $0
	  Store $19 $5
	  $20 = StageOutput 1: Smooth $0
	  Store $20 $12
	  $21 = StageOutput 2: Smooth $1
	  Store $21 $11
	  $22 = StageOutput 3: Smooth $0
	  Store $22 $17
	  $23 = StageOutput 4: Smooth $0
	  Store $23 $18
	}
	)");
};
