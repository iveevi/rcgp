#include "common.hpp"
#include "common_resources.hpp"

#define SUITE "tracing"

add_test(vs_empty)
{
	auto vs = $shader(vertex)() {};

	assert_assembly_match(vs, R"(
	block {
	  context {
	    model: vertex shader,
	    name: main,
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
	block {
	  context {
	    model: vertex shader,
	    name: main,
	  }
	  $0 = Float
	  $1 = local $0
	  $2 = 1
	  store $1 $2
	  $3 = Vec4
	  $4 = new $3($1, $1, $1, $1)
	  $5 = local $3
	  store $5 $4
	  $6 = SVPosition
	  store $6 $5
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
	block {
	  context {
	    model: vertex shader,
	    name: main,
	    stage out 0: $0 (smooth),
	    stage out 1: $1 (flat),
	  }
	  $2 = Float
	  $3 = local $2
	  $4 = 1
	  store $3 $4
	  $0 = Vec3
	  $5 = new $0($3, $3, $3)
	  $6 = local $0
	  store $6 $5
	  $7 = stage out($0, 0, smooth)
	  store $7 $6
	  $8 = UInt32
	  $9 = local $8
	  $10 = 4
	  store $9 $10
	  $11 = local $8
	  $12 = 1
	  store $11 $12
	  $1 = UVec2
	  $13 = new $1($11, $9)
	  $14 = local $1
	  store $14 $13
	  $15 = stage out($1, 1, flat)
	  store $15 $14
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
	block {
	  context {
	    model: vertex shader,
	    name: main,
	    stage in 0: $0,
	    stage out 0: $0 (smooth),
	  }
	  $1 = Vec4
	  $2 = Float
	  $0 = Vec3
	  $3 = local $0
	  $4 = stage in($0, 0)
	  $5 = local $2
	  $6 = 1
	  store $5 $6
	  $7 = new $1($4, $5)
	  $8 = local $1
	  store $8 $7
	  $9 = SVPosition
	  store $9 $8
	  $10 = stage out($0, 0, smooth)
	  store $10 $4
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
	block {
	  context {
	    model: vertex shader,
	    name: main,
	    stage in 0: $0,
	    stage out 0: $0 (smooth),
	    resource: {$1},
	  }
	  $2 = Vec4
	  $3 = Float
	  $0 = Vec3
	  $4 = local $0
	  $5 = FMat4x4
	  $6 = local $5
	  $7 = local $5
	  $8 = local $5
	  $9 = local $5
	  $10 = local $5
	  $11 = local $5
	  $12 = View($5, $5, $5)
	  $1 = push_constant($12, nil:0, Std430)
	  $13 = field $1:0
	  $14 = field $1:1
	  $15 = field $1:2
	  $16 = stage in($0, 0)
	  $17 = local $3
	  $18 = 1
	  store $17 $18
	  $19 = new $2($16, $17)
	  $20 = local $2
	  store $20 $19
	  $21 = mul($13, $20)
	  $22 = mul($15, $14)
	  $23 = mul($22, $21)
	  $24 = SVPosition
	  store $24 $23
	  $25 = new $0($21)
	  $26 = local $0
	  store $26 $25
	  $27 = stage out($0, 0, smooth)
	  store $27 $26
	}
	)");
};

add_test(sr_return_primitives)
{
	auto sr = $subroutine(sr)(f32 x, u32 y) {
		return std::tuple { float3(x), uint2(y, 13) };
	};
	
	assert_assembly_match(sr, R"(
	block {
	  context {
	    model: subroutine,
	    name: sr,
	    argument 0: $0,
	    argument 1: $1,
	    return 0: $2,
	    return 1: $3,
	  }
	  $3 = UVec2
	  $2 = Vec3
	  $0 = Float
	  $1 = UInt32
	  $4 = local $1
	  $5 = local $0
	  $6 = argument $0:0
	  $7 = argument $1:1
	  $8 = new $2($6, $6, $6)
	  $9 = local $2
	  store $9 $8
	  $10 = local $1
	  $11 = 13
	  store $10 $11
	  $12 = new $3($7, $10)
	  $13 = local $3
	  store $13 $12
	  $14 = return($2, 0)
	  store $14 $9
	  $15 = return($3, 1)
	  store $15 $13
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
	block {
	  context {
	    model: subroutine,
	    name: sr,
	    argument 0: $0,
	    return 0: $1,
	  }
	  $2 = Vec3
	  $0 = Float
	  $3 = local $0
	  $4 = argument $0:0
	  $5 = local $0
	  $6 = 0
	  store $5 $6
	  $7 = new $2($5, $5, $5)
	  $8 = local $2
	  store $8 $7
	  $9 = local $0
	  $10 = 1
	  store $9 $10
	  $11 = local $0
	  $12 = 1
	  store $11 $12
	  $13 = new $2($11, $4, $9)
	  $14 = local $2
	  store $14 $13
	  $15 = normalize($14)
	  $1 = Ray($2, $2)
	  $16 = return($1, 0)
	  $17 = new $1($8, $15)
	  store $16 $17
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
	block {
	  context {
	    model: vertex shader,
	    name: main,
	    stage out 0: $0 (smooth),
	    stage out 1: $0 (smooth),
	    stage out 2: $1 (smooth),
	    stage out 3: $0 (smooth),
	    stage out 4: $0 (smooth),
	  }
	  $2 = Ray($0, $0)
	  $1 = UVec2
	  $3 = UInt32
	  $0 = Vec3
	  $4 = Float
	  $5 = local $4
	  $6 = 1
	  store $5 $6
	  $7 = local $0
	  @sr1($5; $7)
	  $8 = local $0
	  $9 = local $3
	  $10 = 2
	  store $9 $10
	  $11 = local $4
	  $12 = 1
	  store $11 $12
	  $13 = local $0
	  $14 = local $1
	  @sr2($11, $9; $13, $14)
	  $15 = local $1
	  $16 = local $0
	  $17 = local $4
	  $18 = 2
	  store $17 $18
	  $19 = local $2
	  @sr3($17; $19)
	  $20 = local $0
	  $21 = local $0
	  $22 = field $19:0
	  $23 = field $19:1
	  $24 = stage out($0, 0, smooth)
	  store $24 $7
	  $25 = stage out($0, 1, smooth)
	  store $25 $14
	  $26 = stage out($1, 2, smooth)
	  store $26 $13
	  $27 = stage out($0, 3, smooth)
	  store $27 $22
	  $28 = stage out($0, 4, smooth)
	  store $28 $23
	}
	)");
};
