// Lock the exact paper-fidelity shape of the rasterization-combinator
// shader-I/O diagnostic: type-aliased rendering, parenthesised type lists,
// and `@<idx>` position. See sections/06_evaluation.tex listing
// error_shader_io.errtxt.

#include <rcgp.hpp>

using namespace rcgp;

void io_mismatch(const Device &device, const ShaderCompiler &compiler, const RenderState &rs)
{
	auto vs = $shader(vertex)(ClipPosition clip) {
		clip = float4(0.0f);
		return std::tuple {
			Smooth(float4(1.0f)),
			Smooth(float3(1.0f)),
			Smooth(float3(1.0f))
		};
	};

	auto fs = $shader(fragment)(float3 a, float3 b, float2 c)
	{
		return float4(a + b, c.x);
	};

	auto pipeline = RasterizationCombinator <Topology::eTriangleList> {
		.device = device,
		.compiler = compiler,
		.render_state = rs,
	} (vs, fs);
}
