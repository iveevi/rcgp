#include <rcgp.hpp>

using namespace rcgp;

// Resources
static UniformBuffer <float4> params_a;
static UniformBuffer <float4> params_b;
static PushConstant <u32> pc_size;

// Valid: bind_pipeline | bind all resources | dispatch
void compute_full(
	const Device &device, const ShaderCompiler &compiler,
	const BoundDescriptor <params_a> &bound_a,
	const BoundDescriptor <params_b> &bound_b,
	const ResourceTypeFor <pc_size> &pc
)
{
	auto cs = $shader(compute)(
		$contracts((a, params_a), (b, params_b), (p, pc_size)),
		WorkGroup <256> group
	) {};

	auto pipeline = ComputeCombinator {
		.device = device,
		.compiler = compiler,
	} (cs);

	auto cmds = nullptr
		| bind_pipeline(pipeline)
		| bind_descriptors(bound_a, bound_b)
		| bind_push_constants <pc_size> (pc)
		| dispatch(64);
}

// Valid: no resources, just bind_pipeline | dispatch
void compute_no_resources(const Device &device, const ShaderCompiler &compiler)
{
	auto cs = $shader(compute)(WorkGroup <1> group) {};

	auto pipeline = ComputeCombinator {
		.device = device,
		.compiler = compiler,
	} (cs);

	auto cmds = nullptr
		| bind_pipeline(pipeline)
		| dispatch(1);
}

// Valid: two dispatches chained with a barrier
void compute_multi_dispatch(
	const Device &device, const ShaderCompiler &compiler,
	const BoundDescriptor <params_a> &bound_a,
	const BoundDescriptor <params_b> &bound_b,
	const ResourceTypeFor <pc_size> &pc,
	const ResourceTypeFor <params_a> &a_buffer
)
{
	auto cs1 = $shader(compute)(
		$contracts((a, params_a), (p, pc_size)),
		WorkGroup <256> group
	) {};

	auto cs2 = $shader(compute)(
		$contracts((a, params_a), (b, params_b), (p, pc_size)),
		WorkGroup <256> group
	) {};

	auto pipeline1 = ComputeCombinator {
		.device = device,
		.compiler = compiler,
	} (cs1);

	auto pipeline2 = ComputeCombinator {
		.device = device,
		.compiler = compiler,
	} (cs2);

	auto cmds = nullptr
		| bind_pipeline(pipeline1)
		| bind_descriptors(bound_a)
		| bind_push_constants <pc_size> (pc)
		| dispatch(64)
		| barrier <params_a> (a_buffer, ComputeWrite, ComputeRead)
		| bind_pipeline(pipeline2)
		| bind_descriptors(bound_a, bound_b)
		| bind_push_constants <pc_size> (pc)
		| dispatch(64);
}

// Invalid: dispatch without binding descriptors
void compute_missing_descriptors(
	const Device &device, const ShaderCompiler &compiler,
	const ResourceTypeFor <pc_size> &pc
)
{
	auto cs = $shader(compute)(
		$contracts((a, params_a), (p, pc_size)),
		WorkGroup <256> group
	) {};

	auto pipeline = ComputeCombinator {
		.device = device,
		.compiler = compiler,
	} (cs);

	auto cmds = nullptr
		| bind_pipeline(pipeline)
		| bind_push_constants <pc_size> (pc)
		| dispatch(64);
}
