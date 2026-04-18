#include <rcgp.hpp>

using namespace rcgp;

static UniformBuffer <float4> params_a;
static UniformBuffer <float4> params_b;
static PushConstant <u32> pc_size;

// Valid: both arms produce identical Commands types
void branch_matching_arms(
	const Device &device, const ShaderCompiler &compiler,
	const BoundDescriptor <params_a> &bound_a,
	const ResourceTypeFor <pc_size> &pc,
	bool flag
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
		| bind_descriptors(bound_a)
		| bind_push_constants <pc_size> (pc)
		| branch(flag,
			[&] { return dispatch(64); },
			[&] { return dispatch(128); });
}

// Invalid: arms produce different Commands types (one dispatches, the other binds a descriptor)
void branch_divergent_arms(
	const Device &device, const ShaderCompiler &compiler,
	const BoundDescriptor <params_a> &bound_a,
	const BoundDescriptor <params_b> &bound_b,
	const ResourceTypeFor <pc_size> &pc,
	bool flag
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
		| bind_descriptors(bound_a)
		| bind_push_constants <pc_size> (pc)
		| branch(flag,
			[&] { return dispatch(64); },
			[&] { return bind_descriptors(bound_b) | dispatch(64); });
}
