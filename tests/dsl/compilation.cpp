#include "common.hpp"
#include "common_resources.hpp"

#define SUITE "compilation"

// Compilation:
// Testing that shader modules compile through the full DSL -> GLSL -> SPIR-V
// pipeline. This requires manually applying resource allocation maps before
// generating GLSL, since we don't have a Device to call apply_gvrs.

static ShaderCompiler compiler;

template <typename GVRs, typename ... Shaders>
void apply_bindings(const GVRs &, Shaders &... shaders)
{
	using descriptor_gvrs = descriptable_resources_t <GVRs>;
	using push_constant_gvrs = push_constant_resources_t <GVRs>;

	auto [_, gamap] = wrappers_to_gamap(descriptor_gvrs());
	(shaders->apply_group_allocation_map(gamap), ...);

	auto [pcrs, pcmap] = wrappers_to_pcs(push_constant_gvrs());
	(shaders->apply_push_constant_allocation_map(pcmap), ...);
}

template <typename Shader>
void apply_bindings_single(Shader &shader)
{
	using S = std::remove_reference_t <Shader>;
	apply_bindings(S::gvrs, shader);
}

template <typename Shader>
void assert_compiles(Shader &shader)
{
	using S = std::remove_reference_t <Shader>;
	constexpr auto stage = S::stage;
	auto glsl = generate_glsl(shader);
	auto spirv = compiler.glsl_to_spirv(glsl, stage);
	if (spirv.empty()) mark_fail;
}

add_test(vs_empty)
{
	auto vs = $shader(vertex)() {};
	assert_compiles(vs);
};

add_test(vs_clip_position)
{
	auto vs = $shader(vertex)(ClipPosition clip) {
		clip = float4(0.0f);
	};
	assert_compiles(vs);
};

add_test(vs_smooth_outputs)
{
	auto vs = $shader(vertex)(ClipPosition clip) {
		clip = float4(0.0f);
		return Smooth(float3(1.0f));
	};
	assert_compiles(vs);
};

add_test(vs_flat_outputs)
{
	auto vs = $shader(vertex)(ClipPosition clip) {
		clip = float4(0.0f);
		return Flat(uint2(1, 4));
	};
	assert_compiles(vs);
};

add_test(vs_mixed_outputs)
{
	auto vs = $shader(vertex)(ClipPosition clip) {
		clip = float4(0.0f);
		return std::tuple {
			Smooth { float3(1.0f) },
			Flat { uint2(1, 4) },
		};
	};
	assert_compiles(vs);
};

add_test(vs_attribute_streams)
{
	auto vs = $shader(vertex)(
		$contracts(
			(pos, fwd::position),
			(nrm, fwd::normal),
			(uv, fwd::uv)
		),
		ClipPosition clip
	) {
		clip = float4(pos, 1.0f);
		return std::tuple {
			Smooth <float3> { pos },
			Smooth <float3> { nrm },
			Smooth <float2> { uv },
		};
	};
	apply_bindings_single(vs);
	assert_compiles(vs);
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
	apply_bindings_single(vs);
	assert_compiles(vs);
};

add_test(vs_storage_buffer)
{
	auto vs = $shader(vertex)(
		$contracts((positions, meshlets::positions)),
		ClipPosition clip
	) {
		clip = positions[u32(0)];
	};
	apply_bindings_single(vs);
	assert_compiles(vs);
};

add_test(fs_constant_color)
{
	auto fs = $shader(fragment)() { return float4(1.0f, 0.0f, 0.0f, 1.0f); };
	assert_compiles(fs);
};

add_test(fs_interpolated_inputs)
{
	auto fs = $shader(fragment)(float3 color) { return float4(color, 1.0f); };
	assert_compiles(fs);
};

add_test(fs_texture_sampling)
{
	auto fs = $shader(fragment)(
		$contracts((material, fwd::material)),
		float2 uv
	) -> float4
	{
		return material.albedo.sample(uv);
	};
	apply_bindings_single(fs);
	assert_compiles(fs);
};

add_test(fs_push_constant)
{
	auto fs = $shader(fragment)(
		$contracts((pack, struct_sorting::pack)),
		float2 uv
	) -> float4
	{
		float3 color = pack.payload.value + float3(uv, pack.weight);
		return float4(color, 1.0f);
	};
	apply_bindings_single(fs);
	assert_compiles(fs);
};

add_test(fs_storage_buffer_read)
{
	auto fs = $shader(fragment)(
		$contracts((lights, fwd::lights)),
		float3 position,
		float3 normal
	) -> float3
	{
		vec3 color = vec3(0.0f);
		$for (i32 i = 0, i < lights.count, i++) {
			auto light = lights.lights[i];
			auto L = light.position - position;
			auto dist2 = max(dot(L, L), f32(1e-4f));
			auto atten = light.intensity / dist2;
			auto ldir = normalize(L);
			auto n_dot_l = max(dot(normal, ldir), f32(0.0f));
			color += light.color * n_dot_l * atten;
		};
		return color;
	};
	apply_bindings_single(fs);
	assert_compiles(fs);
};

add_test(fs_diffuse_lighting)
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
	apply_bindings_single(fs);
	assert_compiles(fs);
};

add_test(raster_passthrough)
{
	auto vs = $shader(vertex)(ClipPosition clip) {
		clip = float4(0.0f);
		return Smooth(float3(1.0f));
	};
	auto fs = $shader(fragment)(float3 color) { return float4(color, 1.0f); };

	auto gvrs = merge_stage_wrappers(tlist_concat(
		std::remove_reference_t <decltype(vs)> ::gvrs,
		std::remove_reference_t <decltype(fs)> ::gvrs
	));
	apply_bindings(gvrs, vs, fs);
	transfer_io_rates(vs, fs);

	assert_compiles(vs);
	assert_compiles(fs);
};

add_test(raster_multi_io)
{
	auto vs = $shader(vertex)(
		$contracts(
			(pos, fwd::position),
			(nrm, fwd::normal),
			(uv, fwd::uv)
		),
		ClipPosition clip
	) {
		clip = float4(pos, 1.0f);
		return std::tuple {
			Smooth <float3> { pos },
			Smooth <float3> { nrm },
			Smooth <float2> { uv },
		};
	};
	auto fs = $shader(fragment)(float3 position, float3 normal, float2 uv) -> float3 {
		return position + normal + float3(uv, 0);
	};

	auto gvrs = merge_stage_wrappers(tlist_concat(
		std::remove_reference_t <decltype(vs)> ::gvrs,
		std::remove_reference_t <decltype(fs)> ::gvrs
	));
	apply_bindings(gvrs, vs, fs);
	transfer_io_rates(vs, fs);

	assert_compiles(vs);
	assert_compiles(fs);
};

add_test(raster_shared_push_constant)
{
	auto vs = $shader(vertex)(
		$contracts(
			(view, fwd::view),
			(pos, fwd::position),
			(nrm, fwd::normal),
			(uv, fwd::uv)
		),
		ClipPosition cpos
	)
	{
		float4 wpos = view.model * float4(pos, 1);
		cpos = view.proj * view.view * wpos;
		return std::tuple {
			Smooth <float3> { float3(wpos) },
			Smooth <float3> { nrm },
			Smooth <float2> { uv },
		};
	};
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

	auto gvrs = merge_stage_wrappers(tlist_concat(
		std::remove_reference_t <decltype(vs)> ::gvrs,
		std::remove_reference_t <decltype(fs)> ::gvrs
	));
	apply_bindings(gvrs, vs, fs);
	transfer_io_rates(vs, fs);

	assert_compiles(vs);
	assert_compiles(fs);
};

add_test(raster_vertex_only)
{
	auto vs = $shader(vertex)(
		$contracts(
			(view, fwd::view),
			(pos, fwd::position)
		),
		ClipPosition cpos
	) {
		cpos = view.proj * view.view * view.model * float4(pos, 1);
	};
	apply_bindings_single(vs);
	assert_compiles(vs);
};

add_test(cs_empty)
{
	auto cs = $shader(compute)(WorkGroup <1> group) {};
	assert_compiles(cs);
};

add_test(cs_workgroup_sizes)
{
	auto cs1 = $shader(compute)(WorkGroup <8, 8> group) {};
	auto cs2 = $shader(compute)(WorkGroup <256> group) {};
	auto cs3 = $shader(compute)(WorkGroup <8, 8, 4> group) {};
	assert_compiles(cs1);
	assert_compiles(cs2);
	assert_compiles(cs3);
};

add_test(cs_push_constant)
{
	auto cs = $shader(compute)(
		$contracts((view, meshlets::view)),
		WorkGroup <1> group
	) {
		float4x4 vp = view.view_proj;
	};
	apply_bindings_single(cs);
	assert_compiles(cs);
};

add_test(cs_storage_buffer)
{
	auto cs = $shader(compute)(
		$contracts((positions, meshlets::positions)),
		WorkGroup <64> group
	) {
		float4 p = positions[u32(0)];
	};
	apply_bindings_single(cs);
	assert_compiles(cs);
};

add_test(cs_array_length)
{
	auto cs = $shader(compute)(
		$contracts((positions, meshlets::positions)),
		WorkGroup <1> group
	) {
		i32 len = positions.length();
	};
	apply_bindings_single(cs);
	assert_compiles(cs);
};

add_test(cs_loop_reduction)
{
	auto cs = $shader(compute)(
		$contracts((positions, meshlets::positions)),
		WorkGroup <1> group
	) {
		float4 sum = float4(0.0f);
		i32 len = positions.length();
		$for (i32 i = 0, i < len, i++) {
			sum += positions[i];
		};
	};
	apply_bindings_single(cs);
	assert_compiles(cs);
};

add_test(ts_minimal)
{
	auto ts = $shader(task)(TaskGroup <1> group) {
		TaskPayload <float3> payload;
		group.dispatch_mesh_groups(1u, 1u, 1u);
		return payload;
	};
	assert_compiles(ts);
};

add_test(ts_frustum_culling)
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
	apply_bindings_single(ts);
	assert_compiles(ts);
};

add_test(ms_minimal)
{
	auto ms = $shader(mesh)(TaskPayload <float3> payload, WorkGroup <1> group) {
		MeshletPayload <MeshPrimitive::eTriangles, 64, 126, meshlets::MeshOutputs> mp;
		return mp;
	};
	assert_compiles(ms);
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
	apply_bindings_single(ms);
	assert_compiles(ms);
};

add_test(mesh_pipeline_minimal)
{
	auto ts = $shader(task)(TaskGroup <1> group) {
		TaskPayload <float3> payload;
		group.dispatch_mesh_groups(1u, 1u, 1u);
		return payload;
	};
	auto ms = $shader(mesh)(TaskPayload <float3> payload, WorkGroup <1> group) {
		MeshletPayload <MeshPrimitive::eTriangles, 64, 126, meshlets::MeshOutputs> mp;
		return mp;
	};
	auto fs = $shader(fragment)(float3 color) { return float4(color, 1.0f); };

	auto gvrs = merge_stage_wrappers(tlist_concat(
		std::remove_reference_t <decltype(ts)> ::gvrs,
		std::remove_reference_t <decltype(ms)> ::gvrs,
		std::remove_reference_t <decltype(fs)> ::gvrs
	));
	apply_bindings(gvrs, ts, ms, fs);

	assert_compiles(ts);
	assert_compiles(ms);
	assert_compiles(fs);
};

add_test(mesh_pipeline_meshlets)
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

	auto fs = $shader(fragment)(float3 color) { return float4(color, 1.0f); };

	auto gvrs = merge_stage_wrappers(tlist_concat(
		std::remove_reference_t <decltype(ts)> ::gvrs,
		std::remove_reference_t <decltype(ms)> ::gvrs,
		std::remove_reference_t <decltype(fs)> ::gvrs
	));
	apply_bindings(gvrs, ts, ms, fs);

	assert_compiles(ts);
	assert_compiles(ms);
	assert_compiles(fs);
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

	auto gvrs = merge_stage_wrappers(tlist_concat(
		std::remove_reference_t <decltype(rgen)> ::gvrs,
		std::remove_reference_t <decltype(miss)> ::gvrs,
		std::remove_reference_t <decltype(chit)> ::gvrs
	));
	apply_bindings(gvrs, rgen, miss, chit);

	assert_compiles(rgen);
	assert_compiles(chit);
	assert_compiles(miss);
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
		$contracts((radiance, receiver <rt::reflection>)),
		float2 bary
	) {
		radiance = float3(bary.x, bary.y, f32(1));
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

	auto gvrs = merge_stage_wrappers(tlist_concat(
		std::remove_reference_t <decltype(rgen)> ::gvrs,
		std::remove_reference_t <decltype(miss_radiance)> ::gvrs,
		std::remove_reference_t <decltype(miss_occlusion)> ::gvrs,
		std::remove_reference_t <decltype(miss_reflection)> ::gvrs,
		std::remove_reference_t <decltype(chit_radiance)> ::gvrs,
		std::remove_reference_t <decltype(chit_reflection)> ::gvrs
	));
	apply_bindings(gvrs, rgen, miss_radiance, miss_occlusion, miss_reflection, chit_radiance, chit_reflection);

	assert_compiles(rgen);
	assert_compiles(chit_radiance);
	assert_compiles(chit_reflection);
	assert_compiles(miss_radiance);
	assert_compiles(miss_occlusion);
	assert_compiles(miss_reflection);
};

add_test(sr_inline_call)
{
	auto sr = $subroutine(sr)(f32 x) {
		return float3(x);
	};

	auto vs = $shader(vertex, &)() {
		auto r = $use(sr)(1);
		return r;
	};
	apply_bindings_single(vs);
	assert_compiles(vs);
};

add_test(sr_dependency_chain)
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
	apply_bindings_single(cs);
	assert_compiles(cs);
};

add_test(sr_shared_across_stages)
{
	auto lighting = $subroutine(lighting)(float3 normal, float3 light_dir) {
		return max(dot(normal, light_dir), f32(0.0f));
	};

	auto vs = $shader(vertex, &)(ClipPosition clip) {
		clip = float4(0.0f);
		auto n_dot_l = $use(lighting)(float3(0, 1, 0), float3(0, 1, 0));
		return Smooth(float3(n_dot_l));
	};

	auto fs = $shader(fragment, &)(float3 color) -> float4 {
		auto n_dot_l = $use(lighting)(float3(0, 0, 1), float3(0, 1, 0));
		return float4(color * n_dot_l, 1.0f);
	};

	auto gvrs = merge_stage_wrappers(tlist_concat(
		std::remove_reference_t <decltype(vs)> ::gvrs,
		std::remove_reference_t <decltype(fs)> ::gvrs
	));
	apply_bindings(gvrs, vs, fs);
	transfer_io_rates(vs, fs);

	assert_compiles(vs);
	assert_compiles(fs);
};

add_test(bufref_array)
{
	auto fs = $shader(fragment)(
		$contracts((geo, bufref::geometry)),
		float3 position
	) -> float3
	{
		auto verts = geo[u32(0)];
		float3 p = verts.positions[u32(0)];
		float3 n = verts.normals[u32(1)];
		float2 uv = verts.uvs[u32(2)];
		return p + n + float3(uv, 0);
	};
	apply_bindings_single(fs);
	assert_compiles(fs);
};

add_test(bufref_single)
{
	auto vs = $shader(vertex)(
		$contracts((scene, bufref::scene)),
		ClipPosition clip
	) {
		float4x4 xform = scene.transform;
		clip = xform * float4(0, 0, 0, 1);
	};
	apply_bindings_single(vs);
	assert_compiles(vs);
};

add_test(struct_ordering)
{
	auto fs = $shader(fragment)(
		$contracts((pack, struct_sorting::pack)),
		float2 uv
	) -> float4
	{
		float3 color = pack.payload.value + float3(uv, pack.weight);
		return float4(color, 1.0f);
	};
	apply_bindings_single(fs);
	assert_compiles(fs);
};

add_test(struct_sanitization)
{
	auto fs = $shader(fragment)(
		$contracts((encoder, sanitization::encoder)),
		float2 uv
	) -> float4
	{
		float3 color = encoder.albedo.value + float3(uv, encoder.roughness.value);
		return float4(color, 1.0f);
	};
	apply_bindings_single(fs);
	assert_compiles(fs);
};

add_test(scalar_layout)
{
	auto cs = $shader(compute)(
		$contracts(
			(tris, meshlets::buffers.triangles),
			(colors, meshlets::buffers.colors)
		),
		WorkGroup <1> group
	) {
		uint3 tri = tris[u32(0)];
		float3 color = colors[u32(0)];
	};
	apply_bindings_single(cs);
	assert_compiles(cs);
};
