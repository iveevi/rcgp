// Lock the exact paper-fidelity shape of the commands-combinator unresolved-
// dependency diagnostic. See sections/06_evaluation.tex listing
// error_command_dependencies.errtxt.

#include <rcgp.hpp>

using namespace rcgp;

static StorageBuffer <array <float4>> lights;

void draw_before_binding_lights(
	const Device &device, const ShaderCompiler &compiler, const RenderState &rs,
	const BoundDescriptor <lights> &bound)
{
	auto vs = $shader(vertex)(ClipPosition clip) { clip = float4(0); };
	auto fs = $shader(fragment)(
		$contracts((L, lights))
	) {
		return float4(L[0]);
	};

	auto pipeline = RasterizationCombinator <Topology::eTriangleList> {
		.device = device,
		.compiler = compiler,
		.render_state = rs,
	} (vs, fs);

	// Forgotten: bind_descriptors(bound) before draw — sentinel must fire,
	// naming `lights` as the unresolved dependency.
	auto cmds = nullptr
		| bind_pipeline(pipeline)
		| draw(3);
}
