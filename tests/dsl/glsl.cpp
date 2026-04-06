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
	    vec3 lvar2;
	    lvar2 = vec3(lvar1);
	    lout0 = lvar2;
	    uint lvar3;
	    lvar3 = 4;
	    uint lvar4;
	    lvar4 = 1;
	    uvec2 lvar5;
	    lvar5 = uvec2(lvar4, lvar3);
	    uvec2 lvar6;
	    lvar6 = uvec2(lvar5);
	    lout1 = lvar6;
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

	#extension GL_EXT_scalar_block_layout : require

	layout (location = 0) in vec3 lin0;

	layout (location = 0) smooth out vec3 lout0;

	void main()
	{
	    vec3 lvar0;
	    lvar0 = vec3(lin0);
	    float lvar1;
	    lvar1 = 1;
	    vec4 lvar2;
	    lvar2 = vec4(lvar0, lvar1);
	    gl_Position = lvar2;
	    vec3 lvar3;
	    lvar3 = vec3(lvar0);
	    lout0 = lvar3;
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
	    lvar0 = vec2(lin2);
	    vec3 lvar1;
	    lvar1 = vec3(lin1);
	    vec3 lvar2;
	    lvar2 = vec3(lin0);
	    vec3 lvar3;
	    lvar3 = vec3(lvar2);
	    lout0 = lvar3;
	    vec3 lvar4;
	    lvar4 = vec3(lvar1);
	    lout1 = lvar4;
	    vec2 lvar5;
	    lvar5 = vec2(lvar0);
	    lout2 = lvar5;
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
		$for (i32 i = 0, i < lights.count, i++) {
			auto light = lights.lights[i];
			auto L = light.position - position;
			auto dist2 = max(dot(L, L), f32(1e-4f));
			auto atten = light.intensity / dist2;

			auto ldir = normalize(L);
			auto n_dot_l = max(dot(normal, ldir), f32(0.0f));

			color += (base * n_dot_l) * light.color * atten;
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
	    uint lvar0;
	    lvar0 = arg1;
	    float lvar1;
	    lvar1 = arg0;
	    float lvar2;
	    lvar2 = lvar1;
	    vec3 lvar3;
	    lvar3 = vec3(lvar2, lvar2, lvar2);
	    uint lvar4;
	    lvar4 = 13;
	    uvec2 lvar5;
	    lvar5 = uvec2(lvar0, lvar4);
	    uvec2 lvar6;
	    lvar6 = uvec2(lvar5);
	    vec3 lvar7;
	    lvar7 = vec3(lvar3);
	    ret0 = lvar7;
	    ret1 = lvar6;
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
	    lvar0 = arg0;
	    float lvar1;
	    lvar1 = 0;
	    vec3 lvar2;
	    lvar2 = vec3(lvar1, lvar1, lvar1);
	    float lvar3;
	    lvar3 = 1;
	    float lvar4;
	    lvar4 = 1;
	    vec3 lvar5;
	    lvar5 = vec3(lvar4, lvar0, lvar3);
	    vec3 lvar6;
	    lvar6 = normalize(lvar5);
	    vec3 lvar7;
	    lvar7 = vec3(lvar6);
	    vec3 lvar8;
	    lvar8 = vec3(lvar7);
	    vec3 lvar9;
	    lvar9 = vec3(lvar2);
	    vec3 lvar10;
	    lvar10 = vec3(lvar9);
	    vec3 lvar11;
	    lvar11 = vec3(lvar8);
	    vec3 lvar12;
	    lvar12 = vec3(lvar10);
	    ret0 = Ray(lvar12, lvar11);
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

add_test(sr_direct_invocation)
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
		auto r1 = sr1(1);
		auto [r2a, r2b] = sr2(1.0f, 2);
		auto r3 = sr3(2.0f);
		return std::tuple { r1, r2a, r2b, r3.origin, r3.direction };
	};

	assert_glsl_match_file(vs, "glsl/sr_invocation.glsl");
};

add_test(sr_direct_dependencies)
{
	auto saxpy = $subroutine(b)(f32 alpha, f32 x, f32 y) {
		return alpha * x + y;
	};

	auto saxpy2 = $subroutine(c, &)(float2 alpha, float2 x, float2 y) {
		return float2(
			saxpy(alpha.x, x.x, y.y),
			saxpy(alpha.y, x.y, y.y)
		);
	};

	auto saxpy3 = $subroutine(a, &)(float4 alpha, float4 x, float4 y) {
		return float4(
			saxpy2(alpha.xy, x.xy, y.yy),
			saxpy2(alpha.zw, x.zw, y.zw)
		);
	};

	auto cs = $shader(compute, &)(WorkGroup <1> group) {
		float4 x = float4(1);
		float4 y = float4(2);
		float4 alpha = float4(3);
		auto z = saxpy3(alpha, x, y);
	};

	assert_glsl_match_file(cs, "glsl/sr_dependencies.glsl");
};

add_test(for_loop)
{
	auto sr = $subroutine(sr)(i32 limit, i32 step) {
		f32 sum = 0;
		$for (i32 i = 0, i < limit, i += step) {
			sum += i;
		};

		return sum;
	};

	assert_glsl_match(sr, R"(
	void sr(int arg0, int arg1, out float ret0)
	{
	    int lvar0;
	    lvar0 = arg1;
	    int lvar1;
	    lvar1 = arg0;
	    float lvar2;
	    lvar2 = 0;
	    int lvar3;
	    lvar3 = 0;
	    while (true) {
	        int lvar4;
	        lvar4 = lvar1;
	        int lvar5;
	        lvar5 = lvar3;
	        bool lvar6;
	        lvar6 = (lvar5 < lvar4);
	        bool lvar7;
	        lvar7 = (!lvar6);
	        bool lvar8;
	        lvar8 = lvar7;
	        bool lvar9;
	        lvar9 = lvar8;
	        if (lvar9) {
	            break;
	        }
	        float lvar10;
	        lvar10 = lvar3;
	        float lvar11;
	        lvar11 = (lvar2 + lvar10);
	        lvar2 = lvar11;
	        int lvar12;
	        lvar12 = (lvar3 + lvar0);
	        lvar3 = lvar12;
	    }
	    ret0 = lvar2;
	}
	)");
};

add_test(branching)
{
	auto sr = $subroutine(sr)() {
		i32 c = 12;
		$if (c > 11) {
			c += 1;
		} $elif (c < 11 and c > 5) {
			c += 2;
		} $else {
			c += 3;
		};
	};
	
	assert_glsl_match_file(sr, "glsl/sr_branching.glsl");
};

add_test(struct_ordering)
{
	auto fs = $shader(fragment)(
		$contracts((pack, struct_sorting::pack)),
		float2 uv
	) -> float4 {
		float3 color = pack.payload.value + float3(uv, pack.weight);
		return float4(color, 1.0f);
	};

	assert_glsl_match_file(fs, "glsl/struct_ordering.glsl");
};

add_test(struct_sanitization)
{
	auto fs = $shader(fragment)(
		$contracts((encoder, sanitization::encoder)),
		float2 uv
	) -> float4 {
		float3 color = encoder.albedo.value + float3(uv, encoder.roughness.value);
		return float4(color, 1.0f);
	};

	assert_glsl_match_file(fs, "glsl/struct_sanitization.glsl");
};

add_test(array_length_unsized)
{
	auto cs = $shader(compute)(
		$contracts((positions, meshlets::positions)),
		WorkGroup <1> group
	) {
		i32 len = positions.length();
	};

	assert_glsl_match(cs, R"(
	#version 460

	#extension GL_EXT_scalar_block_layout : require

	layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

	layout (std430, set = 0, binding = 0) readonly buffer Buffer0x0 {
	    vec4 value[];
	} r0b0;

	void main()
	{
	    int lvar0;
	    lvar0 = r0b0.value.length();
	}
	)");
};

add_test(array_length_sized)
{
	auto cs = $shader(compute)(
		$contracts((view, meshlets::view)),
		WorkGroup <1> group
	) {
		i32 len = view.frustum_planes.length();
	};

	assert_glsl_match_file(cs, "glsl/array_length_sized.glsl");
};

add_test(group_allocation_map)
{
	$decl(get_albedo)($contracts((a, mapping::albedo)), float2 uv)
	{
		return a.sample(uv);
	};

	auto fs = $shader(fragment, &)(
		$contracts(
			(texs, mapping::albedo),
			(lights, mapping::lights)
		),
		float2 uv
	) -> float4 {
		return $use(get_albedo)(uv);
	};

	using Lights = stage_wrapper <mapping::lights, ShaderStage::eFragment>;
	using Albedo = stage_wrapper <mapping::albedo, ShaderStage::eFragment>;
	auto gvrs = Tlist <Lights, Albedo> ();
	auto [_, gamap] = wrappers_to_gamap(gvrs);
	fs->apply_group_allocation_map(gamap);
	
	assert_glsl_match_file(fs, "glsl/group_allocation_map.glsl");
};

add_test(rt_single_trace_group)
{
	auto rgen = $shader(raygen)(
		$contracts(
			(tlas, rt::tlas), (img, rt::image),
			(radiance, dispatcher <rt::radiance>)
		),
		LaunchID idx, LaunchSize size
	) {
		uint3 pixel = idx;
		radiance = float3(0);
		radiance.trace(tlas, Ray { float3(0), float3(0, 0, 1) }, f32(0.001f), f32(100.0f));
		img.write(uint2(pixel.xy), float4(radiance, 0));
	};

	auto chit = $shader(chit)(
		$contracts((radiance, receiver <rt::radiance>)),
		float2 bary, PrimitiveID prim
	) {
		radiance = float3(bary.x, bary.y, f32(0));
	};

	auto miss = $shader(miss)($contracts((radiance, receiver <rt::radiance>)))
	{
		radiance = float3(0);
	};

	resolve_trace_groups(rgen, std::tuple { miss }, chit);

	assert_glsl_match_file(rgen, "glsl/rt_single_rgen.glsl");
	assert_glsl_match_file(chit, "glsl/rt_single_chit.glsl");
	assert_glsl_match_file(miss, "glsl/rt_single_miss.glsl");
};

add_test(rt_multi_trace_group)
{
	auto rgen = $shader(raygen)(
		$contracts(
			(tlas, rt::tlas), (img, rt::image),
			(radiance, dispatcher <rt::radiance>)
		),
		LaunchID idx, LaunchSize size
	) {
		uint3 pixel = idx;
		radiance = float3(0);
		radiance.trace(tlas, Ray { float3(0), float3(0, 0, 1) }, f32(0.001f), f32(100.0f));
		img.write(uint2(pixel.xy), float4(radiance, 0));
	};

	auto chit_radiance = $shader(chit)(
		$contracts(
			(tlas, rt::tlas),
			(radiance, receiver <rt::radiance>),
			(shadow, dispatcher <rt::occlusion>),
			(refl, dispatcher <rt::reflection>)
		),
		float2 bary, WorldRayOrigin origin,
		WorldRayDirection direction, HitTime t
	) {
		float3 hit = float3(origin) + float3(direction) * t;

		shadow = 0.0f;
		shadow.trace(tlas, Ray { hit, float3(0, 1, 0) }, f32(0.001f), f32(100.0f));

		float3 V = normalize(float3(origin) - hit);
		float3 N = float3(0, 1, 0);
		float3 R = V - N * f32(2) * dot(V, N);
		refl = float3(0);
		refl.trace(tlas, Ray { hit, R }, f32(0.001f), f32(100.0f));

		f32 shadow_val = shadow;
		float3 refl_color = refl;
		radiance = float3(shadow_val) + refl_color * f32(0.3f);
	};

	auto chit_reflection = $shader(chit)(
		$contracts((radaince, receiver <rt::reflection>)),
		float2 bary
	) {
		radaince = float3(bary.x, bary.y, f32(1));
	};

	auto miss_radiance = $shader(miss)($contracts((r, receiver <rt::radiance>)))
	{ r = float3(0.5f, 0.7f, 1.0f); };

	auto miss_occlusion = $shader(miss)($contracts((s, receiver <rt::occlusion>)))
	{ s = 1.0f; };

	auto miss_reflection = $shader(miss)($contracts((r, receiver <rt::reflection>)))
	{ r = float3(0.1f, 0.1f, 0.1f); };

	resolve_trace_groups(
		rgen,
		std::tuple { miss_radiance, miss_occlusion, miss_reflection },
		chit_radiance, chit_reflection
	);

	assert_glsl_match_file(rgen, "glsl/rt_multi_rgen.glsl");
	assert_glsl_match_file(chit_radiance, "glsl/rt_multi_chit_radiance.glsl");
	assert_glsl_match_file(chit_reflection, "glsl/rt_multi_chit_reflection.glsl");
	assert_glsl_match_file(miss_radiance, "glsl/rt_multi_miss_radiance.glsl");
	assert_glsl_match_file(miss_occlusion, "glsl/rt_multi_miss_occlusion.glsl");
	assert_glsl_match_file(miss_reflection, "glsl/rt_multi_miss_reflection.glsl");
};

add_test(bufref_array)
{
	auto fs = $shader(fragment)(
		$contracts(
			(geo, bufref::geometry)
		),
		float3 position
	) -> float3
	{
		auto verts = geo[u32(0)];
		float3 p = verts.positions[u32(0)];
		float3 n = verts.normals[u32(1)];
		float2 uv = verts.uvs[u32(2)];
		return p + n + float3(uv, 0);
	};

	assert_glsl_match_file(fs, "glsl/bufref_array.glsl");
};

add_test(bufref_single)
{
	auto vs = $shader(vertex)(
		$contracts(
			(scene, bufref::scene)
		),
		ClipPosition clip
	)
	{
		float4x4 xform = scene.transform;
		clip = xform * float4(0, 0, 0, 1);
	};

	assert_glsl_match_file(vs, "glsl/bufref_single.glsl");
};
