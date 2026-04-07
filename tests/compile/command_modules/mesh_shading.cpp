#include <rcgp.hpp>

using namespace rcgp;

// Resources
static PushConstant <float4x4> view;
static UniformBuffer <float4> params;

struct MeshOutputs {
	PerVertex <float3> color;
	$reflection(color);
};

// Valid: bind_pipeline | bind all resources | draw_mesh_tasks
void mesh_full(
	const Device &device, const ShaderCompiler &compiler, const RenderState &rs,
	const BoundDescriptor <params> &bound_params,
	const ResourceTypeFor <view> &pc
)
{
	auto ts = $shader(task)(
		$contracts((v, view)),
		TaskGroup <1> group
	) {
		TaskPayload <float3> payload;
		group.dispatch_mesh_groups(1u, 1u, 1u);
		return payload;
	};
	auto ms = $shader(mesh)(
		$contracts((p, params)),
		TaskPayload <float3> payload,
		WorkGroup <1> group
	) {
		MeshletPayload <MeshPrimitive::eTriangles, 64, 126, MeshOutputs> mp;
		return mp;
	};
	auto fs = $shader(fragment)(float3 color) { return float4(color, 1); };

	auto pipeline = MeshShadingCombinator {
		.device = device,
		.compiler = compiler,
		.render_state = rs,
	} (ts, ms, fs);

	auto cmds = nullptr
		| bind_pipeline(pipeline)
		| bind_descriptors(bound_params)
		| bind_push_constants <view> (pc)
		| draw_mesh_tasks(1);
}

// Valid: no resources, just bind_pipeline | draw_mesh_tasks
void mesh_no_resources(const Device &device, const ShaderCompiler &compiler, const RenderState &rs)
{
	auto ts = $shader(task)(TaskGroup <1> group) {
		TaskPayload <float3> payload;
		group.dispatch_mesh_groups(1u, 1u, 1u);
		return payload;
	};
	auto ms = $shader(mesh)(TaskPayload <float3> payload, WorkGroup <1> group) {
		MeshletPayload <MeshPrimitive::eTriangles, 64, 126, MeshOutputs> mp;
		return mp;
	};
	auto fs = $shader(fragment)(float3 color) { return float4(color, 1); };

	auto pipeline = MeshShadingCombinator {
		.device = device,
		.compiler = compiler,
		.render_state = rs,
	} (ts, ms, fs);

	auto cmds = nullptr
		| bind_pipeline(pipeline)
		| draw_mesh_tasks(1);
}

// Invalid: draw_mesh_tasks without binding descriptors
void mesh_missing_descriptors(
	const Device &device, const ShaderCompiler &compiler, const RenderState &rs,
	const ResourceTypeFor <view> &pc
)
{
	auto ts = $shader(task)(
		$contracts((v, view)),
		TaskGroup <1> group
	) {
		TaskPayload <float3> payload;
		group.dispatch_mesh_groups(1u, 1u, 1u);
		return payload;
	};
	auto ms = $shader(mesh)(
		$contracts((p, params)),
		TaskPayload <float3> payload,
		WorkGroup <1> group
	) {
		MeshletPayload <MeshPrimitive::eTriangles, 64, 126, MeshOutputs> mp;
		return mp;
	};
	auto fs = $shader(fragment)(float3 color) { return float4(color, 1); };

	auto pipeline = MeshShadingCombinator {
		.device = device,
		.compiler = compiler,
		.render_state = rs,
	} (ts, ms, fs);

	auto cmds = nullptr
		| bind_pipeline(pipeline)
		| bind_push_constants <view> (pc)
		| draw_mesh_tasks(1);
}
