#include <rcgp.hpp>

using namespace rcgp;

// Resources
static PushConstant <float4x4> mvp;
static UniformBuffer <float4> material;

// Valid: bind_pipeline | bind all resources | draw
void raster_full(
	const Device &device, const ShaderCompiler &compiler, const RenderState &rs,
	const BoundDescriptor <material> &bound_mat,
	const ResourceTypeFor <mvp> &pc
)
{
	auto vs = $shader(vertex)(
		$contracts((v, mvp)),
		ClipPosition clip
	) {
		clip = v * float4(0, 0, 0, 1);
		return Smooth <float3> { float3(1) };
	};
	auto fs = $shader(fragment)(
		$contracts((m, material)),
		float3 nrm
	) {
		return float4(nrm, 1);
	};

	auto pipeline = RasterizationCombinator <Topology::eTriangleList> {
		.device = device,
		.compiler = compiler,
		.render_state = rs,
	} (vs, fs);

	auto cmds = nullptr
		| bind_pipeline(pipeline)
		| bind_descriptors(bound_mat)
		| bind_push_constants <mvp> (pc)
		| draw(3);
}

// Valid: bind_pipeline | bind all resources | bind_index_buffer | draw_indexed
void raster_draw_indexed(
	const Device &device, const ShaderCompiler &compiler, const RenderState &rs,
	const BoundDescriptor <material> &bound_mat,
	const ResourceTypeFor <mvp> &pc,
	const IndexBuffer <Topology::eTriangleList, uint32_t> &idx_buf
)
{
	auto vs = $shader(vertex)(
		$contracts((v, mvp)),
		ClipPosition clip
	) {
		clip = v * float4(0, 0, 0, 1);
		return Smooth <float3> { float3(1) };
	};
	auto fs = $shader(fragment)(
		$contracts((m, material)),
		float3 nrm
	) {
		return float4(nrm, 1);
	};

	auto pipeline = RasterizationCombinator <Topology::eTriangleList> {
		.device = device,
		.compiler = compiler,
		.render_state = rs,
	} (vs, fs);

	auto cmds = nullptr
		| bind_pipeline(pipeline)
		| bind_descriptors(bound_mat)
		| bind_push_constants <mvp> (pc)
		| bind_index_buffer <Topology::eTriangleList> (idx_buf)
		| draw_indexed(36);
}

// Valid: no resources, just bind_pipeline | draw
void raster_no_resources(const Device &device, const ShaderCompiler &compiler, const RenderState &rs)
{
	auto vs = $shader(vertex)(ClipPosition clip) { clip = float4(0); };
	auto fs = $shader(fragment)() { return float4(1); };

	auto pipeline = RasterizationCombinator <Topology::eTriangleList> {
		.device = device,
		.compiler = compiler,
		.render_state = rs,
	} (vs, fs);

	auto cmds = nullptr
		| bind_pipeline(pipeline)
		| draw(3);
}

// Valid: vertex-only pipeline (depth pass)
void raster_vertex_only(
	const Device &device, const ShaderCompiler &compiler, const RenderState &rs,
	const ResourceTypeFor <mvp> &pc
)
{
	auto vs = $shader(vertex)(
		$contracts((v, mvp)),
		ClipPosition clip
	) {
		clip = v * float4(0, 0, 0, 1);
	};

	auto pipeline = RasterizationCombinator <Topology::eTriangleList> {
		.device = device,
		.compiler = compiler,
		.render_state = rs,
	} (vs);

	auto cmds = nullptr
		| bind_pipeline(pipeline)
		| bind_push_constants <mvp> (pc)
		| draw(36);
}

// Valid: effectless commands compose freely with pipeline
void raster_effectless(const Device &device, const ShaderCompiler &compiler, const RenderState &rs)
{
	auto vs = $shader(vertex)(ClipPosition clip) { clip = float4(0); };
	auto fs = $shader(fragment)() { return float4(1); };

	auto pipeline = RasterizationCombinator <Topology::eTriangleList> {
		.device = device,
		.compiler = compiler,
		.render_state = rs,
	} (vs, fs);

	auto cmds = nullptr
		| set_viewport_scissor(vk::Extent2D(1024, 1024))
		| bind_pipeline(pipeline)
		| draw(3);
}

// Invalid: draw without binding descriptors
void raster_missing_descriptors(
	const Device &device, const ShaderCompiler &compiler, const RenderState &rs,
	const ResourceTypeFor <mvp> &pc
)
{
	auto vs = $shader(vertex)(
		$contracts((v, mvp)),
		ClipPosition clip
	) {
		clip = v * float4(0, 0, 0, 1);
		return Smooth <float3> { float3(1) };
	};
	auto fs = $shader(fragment)(
		$contracts((m, material)),
		float3 nrm
	) {
		return float4(nrm, 1);
	};

	auto pipeline = RasterizationCombinator <Topology::eTriangleList> {
		.device = device,
		.compiler = compiler,
		.render_state = rs,
	} (vs, fs);

	auto cmds = nullptr
		| bind_pipeline(pipeline)
		| bind_push_constants <mvp> (pc)
		| draw(3);
}

// Invalid: draw_indexed without bind_index_buffer
void raster_missing_index_buffer(
	const Device &device, const ShaderCompiler &compiler, const RenderState &rs,
	const BoundDescriptor <material> &bound_mat,
	const ResourceTypeFor <mvp> &pc
)
{
	auto vs = $shader(vertex)(
		$contracts((v, mvp)),
		ClipPosition clip
	) {
		clip = v * float4(0, 0, 0, 1);
		return Smooth <float3> { float3(1) };
	};
	auto fs = $shader(fragment)(
		$contracts((m, material)),
		float3 nrm
	) {
		return float4(nrm, 1);
	};

	auto pipeline = RasterizationCombinator <Topology::eTriangleList> {
		.device = device,
		.compiler = compiler,
		.render_state = rs,
	} (vs, fs);

	auto cmds = nullptr
		| bind_pipeline(pipeline)
		| bind_descriptors(bound_mat)
		| bind_push_constants <mvp> (pc)
		| draw_indexed(36);
}
