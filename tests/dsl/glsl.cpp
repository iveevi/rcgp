#include "common.hpp"
#include "common_resources.hpp"

#define SUITE "glsl"

// GLSL:
// Testing that shader modules and subroutines are generated into reasonable
// GLSL shader code.

add_test(vs_empty)
{
	auto vs = $shader(vertex)() {};

	assert_glsl_match(vs, R"(
	#version 460

	#extension GL_EXT_scalar_block_layout : require

	void main()
	{
	}
	)");
};

add_test(vs_clip)
{
	auto vs = $shader(vertex)(ClipPosition clip)
	{
		clip = float4(1);
	};

	assert_glsl_match(vs, R"(
	#version 460

	#extension GL_EXT_scalar_block_layout : require

	void main()
	{
	    float lvar0;
	    lvar0 = 1;
	    vec4 lvar1;
	    lvar1 = vec4(lvar0, lvar0, lvar0, lvar0);
	    gl_Position = lvar1;
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

	assert_glsl_match(vs, R"(
	#version 460

	#extension GL_EXT_scalar_block_layout : require

	layout (location = 0) smooth out vec3 lout0;
	layout (location = 1) flat out uvec2 lout1;

	void main()
	{
	    float lvar0;
	    lvar0 = 1;
	    vec3 lvar1;
	    lvar1 = vec3(lvar0, lvar0, lvar0);
	    lout0 = lvar1;
	    uint lvar2;
	    lvar2 = 4;
	    uint lvar3;
	    lvar3 = 1;
	    uvec2 lvar4;
	    lvar4 = uvec2(lvar3, lvar2);
	    lout1 = lvar4;
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

	assert_glsl_match(vs, R"(
	#version 460
	
	#extension GL_EXT_scalar_block_layout : require

	layout (location = 0) in vec3 lin0;

	layout (location = 0) smooth out vec3 lout0;

	void main()
	{
	    vec3 lvar0;
	    float lvar1;
	    lvar1 = 1;
	    vec4 lvar2;
	    lvar2 = vec4(lin0, lvar1);
	    gl_Position = lvar2;
	    lout0 = lin0;
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
	
	assert_glsl_match(vs, R"(
	#version 460

	#extension GL_EXT_scalar_block_layout : require

	layout (location = 0) in vec3 lin0;
	layout (location = 1) in vec3 lin1;
	layout (location = 2) in vec2 lin2;

	layout (location = 0) smooth out vec3 lout0;
	layout (location = 1) smooth out vec3 lout1;
	layout (location = 2) smooth out vec2 lout2;

	void main()
	{
	    vec2 lvar0;
	    vec3 lvar1;
	    vec3 lvar2;
	    lout0 = lin0;
	    lout1 = lin1;
	    lout2 = lin2;
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

	assert_glsl_match_file(vs, "glsl/vs_push_constant.glsl");
};

add_test(fr_diffuse_lighting)
{
	auto fs = $shader(fragment)(
		$contracts(lights, material),
		float3 position,
		float3 normal,
		float2 uv
	) -> float3
	{
		vec3 base = material.albedo.sample(uv).xyz;

		vec3 color = vec3(0.0f);
		$for (i32 i = 0, i < lights.count, ++i) {
			auto light = lights.lights[i];
			auto L = light.position - position;
			auto dist2 = max(dot(L, L), f32(1e-4f));
			auto atten = light.intensity / dist2;

			auto ldir = normalize(L);
			auto n_dot_l = max(dot(normal, ldir), f32(0.0f));

			// TODO: += and etc
			color = color + (base * n_dot_l) * light.color * atten;
		};

		return color;
	};

	assert_glsl_match_file(fs, "glsl/fr_diffuse_lighting.glsl");
};

add_test(sr_return_primitives)
{
	auto sr = $subroutine(sr)(f32 x, u32 y) {
		return std::tuple { float3(x), uint2(y, 13) };
	};
	
	assert_glsl_match(sr, R"(
	void sr(float arg0, uint arg1, out vec3 ret0, out uvec2 ret1)
	{
	    uint lvar0;
	    float lvar1;
	    vec3 lvar2;
	    lvar2 = vec3(arg0, arg0, arg0);
	    uint lvar3;
	    lvar3 = 13;
	    uvec2 lvar4;
	    lvar4 = uvec2(arg1, lvar3);
	    ret0 = lvar2;
	    ret1 = lvar4;
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
	
	assert_glsl_match(sr, R"(
	void sr(float arg0, out Ray ret0)
	{
	    float lvar0;
	    float lvar1;
	    lvar1 = 0;
	    vec3 lvar2;
	    lvar2 = vec3(lvar1, lvar1, lvar1);
	    float lvar3;
	    lvar3 = 1;
	    float lvar4;
	    lvar4 = 1;
	    vec3 lvar5;
	    lvar5 = vec3(lvar4, arg0, lvar3);
	    normalize(lvar5);
	    ret0 = Ray(lvar2, normalize(lvar5));
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

	assert_glsl_match_file(vs, "glsl/sr_invocation.glsl");
};

add_test(for_loop)
{
	auto sr = $subroutine(sr)(i32 limit, i32 step) {
		f32 sum = 0;
		$for (i32 i = 0, i < limit, i = i + step) {
			sum = sum + i;
		};

		return sum;
	};

	assert_glsl_match(sr, R"(
	void sr(int arg0, int arg1, out float ret0)
	{
	    int lvar0;
	    int lvar1;
	    float lvar2;
	    lvar2 = 0;
	    int lvar3;
	    lvar3 = 0;
	    while (true) {
	        if ((!(lvar3 < arg0))) {
	            break;
	        }
	        lvar2 = (lvar2 + lvar3);
	        lvar3 = (lvar3 + arg1);
	    }
	    ret0 = lvar2;
	}
	)");
};
