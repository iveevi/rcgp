#include "common.hpp"

using namespace rcgp;

struct View {
	float4x4 model;
	float4x4 view;
	float4x4 proj;

	$reflection(model, view, proj);
};

PushConstant <View> view;

AttributeStream <float3> position;

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
	  $0 = f32
	  $1 = local $0
	  $2 = 1
	  store $1 $2
	  $3 = float4
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
	    thread out 0: $0 (smooth),
	    thread out 1: $1 (flat),
	  }
	  $2 = f32
	  $3 = local $2
	  $4 = 1
	  store $3 $4
	  $0 = float3
	  $5 = new $0($3, $3, $3)
	  $6 = local $0
	  store $6 $5
	  $7 = thread out($0, 0, smooth)
	  store $7 $6
	  $8 = u32
	  $9 = local $8
	  $10 = 4
	  store $9 $10
	  $11 = local $8
	  $12 = 1
	  store $11 $12
	  $1 = uint2
	  $13 = new $1($11, $9)
	  $14 = local $1
	  store $14 $13
	  $15 = thread out($1, 1, flat)
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
	    thread in 0: $0,
	    thread out 0: $0 (-),
	  }
	  $1 = float4
	  $2 = f32
	  $0 = float3
	  $3 = local $0
	  $4 = thread in($0, 0)
	  $5 = local $2
	  $6 = 1
	  store $5 $6
	  $7 = new $1($4, $5)
	  $8 = local $1
	  store $8 $7
	  $9 = SVPosition
	  store $9 $8
	  $10 = thread out($0, 0, -)
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
	    thread in 0: $0,
	    thread out 0: $0 (-),
	    resource: {$1},
	  }
	  $2 = float4
	  $3 = f32
	  $0 = float3
	  $4 = local $0
	  $5 = float4x4
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
	  $16 = thread in($0, 0)
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
	  $27 = thread out($0, 0, -)
	  store $27 $26
	}
	)");
};
