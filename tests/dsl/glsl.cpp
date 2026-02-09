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
		$contracts((p, fwd::position)),
		ClipPosition cpos
	) -> float3
	{
		cpos = float4(p, 1);
		return p;
	};

	assert_glsl_match(vs, R"(
	#version 460
	
	layout (location = 0) in vec3 lin0;
	
	layout (location = 0) smooth out vec3 lout0;
	
	void main()
	{
	    float lvar0;
	    lvar0 = 1;
	    vec4 lvar1;
	    lvar1 = vec4(lin0, lvar0);
	    gl_Position = lvar1;
	    lout0 = lin0;
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
	
	assert_glsl_match(vs, R"(
	#version 460
	
	layout (location = 0) in vec3 lin0;
	layout (location = 1) in vec3 lin1;
	layout (location = 2) in vec2 lin2;
	
	layout (location = 0) smooth out vec3 lout0;
	layout (location = 1) smooth out vec3 lout1;
	layout (location = 2) smooth out vec2 lout2;
	
	void main()
	{
	    lout0 = lin0;
	    lout1 = lin1;
	    lout2 = lin2;
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

	assert_glsl_match_file(vs, "glsl/vs_push_constant.glsl");
};

add_test(fr_diffuse_lighting)
{
	auto fs = $shader(fragment)(
		$contracts(
			(lights, fwd::lights),
			(material, fwd::material)
		),
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

	// TODO: get rid of side-effect (i.e. value based)
	// free intrinsics that are by themselves...
	assert_glsl_match_file(fs, "glsl/fr_diffuse_lighting.glsl");
};

add_test(ts_meshlets)
{
	auto ts = $shader(task)(
		$contracts(
			(view, meshlets::view),
			(meshlets, meshlets::meshlets)
		),
		TaskGroup <1> group
	)
	{
		TaskPayload <meshlets::TaskPayloadData> payload;

		uvec3 gid = group.workgroup_index;
		u32 meshlet_index = gid.y * view.task_group_width + gid.x;
		$if (meshlet_index < view.meshlet_count) {
			auto meshlet = meshlets[meshlet_index];
			vec3 center = vec3(meshlet.bounds);
			f32 radius = meshlet.bounds.w;

			boolean visible = true;
			$for (u32 i = 0, i < 6u, i++) {
				vec4 plane = view.frustum_planes[i];
				vec3 plane_n = vec3(plane);
				f32 dist = dot(plane_n, center) + plane.w;
				$if (dist < -radius) {
					visible = false;
				};
			};

			$if (visible) {
				payload.meshlet = meshlet_index;
				group.dispatch_mesh_groups(1u, 1u, 1u);
			};
		};

		return payload;
	};
	
	assert_glsl_match_file(ts, "glsl/ts_meshlets.glsl");
};

add_test(ms_meshlets)
{
	auto ms = $shader(mesh)(
		$contracts(
			(view, meshlets::view),
			(positions, meshlets::positions),
			(meshlets, meshlets::meshlets)
		),
		$contracts(
			(verts, meshlets::buffers.vertices),
			(tris, meshlets::buffers.triangles),
			(colors, meshlets::buffers.colors)
		),
		TaskPayload <meshlets::TaskPayloadData> payload,
		WorkGroup <1> group
	)
	{
		MeshletPayload <
			MeshPrimitive::eTriangles,
			64, 126,
			meshlets::MeshOutputs
		> out;

		u32 meshlet_index = payload.meshlet;
		auto meshlet = meshlets[meshlet_index];

		out.allocate(meshlet.vertex_count, meshlet.primitive_count);

		$for (u32 v = 0, v < meshlet.vertex_count, v++) {
			u32 global_index = verts[meshlet.vertex_offset + v];
			vec4 pos = positions[global_index];
			out.positions[v] = view.view_proj * pos;
			out.color[v] = colors[meshlet_index];
		};

		$for (u32 p = 0, p < meshlet.primitive_count, p++) {
			out.triangles[p] = tris[meshlet.primitive_offset + p];
		};

		return out;
	};

	assert_glsl_match_file(ms, "glsl/ms_meshlets.glsl");
};

add_test(sr_return_primitives)
{
	auto sr = $subroutine(sr)(f32 x, u32 y) {
		return std::tuple { float3(x), uint2(y, 13) };
	};
	
	assert_glsl_match(sr, R"(
	void sr(float arg0, uint arg1, out vec3 ret0, out uvec2 ret1)
	{
	    vec3 lvar0;
	    lvar0 = vec3(arg0, arg0, arg0);
	    uint lvar1;
	    lvar1 = 13;
	    uvec2 lvar2;
	    lvar2 = uvec2(arg1, lvar1);
	    ret0 = lvar0;
	    ret1 = lvar2;
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
	    lvar0 = 0;
	    vec3 lvar1;
	    lvar1 = vec3(lvar0, lvar0, lvar0);
	    float lvar2;
	    lvar2 = 1;
	    float lvar3;
	    lvar3 = 1;
	    vec3 lvar4;
	    lvar4 = vec3(lvar3, arg0, lvar2);
	    normalize(lvar4);
	    vec3 lvar5;
	    lvar5 = normalize(lvar4);
	    ret0 = Ray(lvar1, lvar5);
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

add_test(sr_dependencies)
{
	auto saxpy = $subroutine(b)(f32 alpha, f32 x, f32 y) {
		return alpha * x + y;
	};

	auto saxpy2 = $subroutine(c, &)(float2 alpha, float2 x, float2 y) {
		return float2(
			$use(saxpy)(alpha.x, x.x, y.y),
			$use(saxpy)(alpha.y, x.y, y.y)
		);
	};
	
	auto saxpy3 = $subroutine(a, &)(float4 alpha, float4 x, float4 y) {
		return float4(
			$use(saxpy2)(alpha.xy, x.xy, y.yy),
			$use(saxpy2)(alpha.zw, x.zw, y.zw)
		);
	};

	auto cs = $shader(compute, &)(WorkGroup <1> group) {
		float4 x = float4(1);
		float4 y = float4(2);
		float4 alpha = float4(3);
		auto z = $use(saxpy3)(alpha, x, y);
	};

	assert_glsl_match_file(cs, "glsl/sr_dependencies.glsl");
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
	    float lvar0;
	    lvar0 = 0;
	    int lvar1;
	    lvar1 = 0;
	    while (true) {
	        bool lvar2;
	        lvar2 = (lvar1 < arg0);
	        bool lvar3;
	        lvar3 = (!lvar2);
	        if (lvar3) {
	            break;
	        }
	        float lvar4;
	        lvar4 = (lvar0 + lvar1);
	        lvar0 = lvar4;
	        int lvar5;
	        lvar5 = (lvar1 + arg1);
	        lvar1 = lvar5;
	    }
	    ret0 = lvar0;
	}
	)");
};

add_test(branching)
{
	auto sr = $subroutine(sr)() {
		i32 c = 12;
		$if (c > 11) {
			c = c + i32(1);
		} $elif (c < 11 and c > 5) {
			c = c + i32(2);
		} $else {
			c = c + i32(3);
		};
	};
	
	assert_glsl_match_file(sr, "glsl/sr_branching.glsl");
};
