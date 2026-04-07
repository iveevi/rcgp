#include <rcgp.hpp>

using namespace rcgp;

// Resources
static RaytracingAccelerationStructure tlas;
static WImage2D image;
static PushConstant <float4x4> view;
static TraceGroup <float3, eOpaque> radiance;

// Valid: bind_pipeline | bind all resources | trace_rays
void rt_full(
	const Device &device, const ShaderCompiler &compiler,
	const BoundDescriptor <tlas> &bound_tlas,
	const BoundDescriptor <image> &bound_image,
	const ResourceTypeFor <view> &pc,
	const ShaderBindingTable &sbt
)
{
	auto rgen = $shader(raygen)(
		$contracts(
			(t, tlas), (img, image),
			(r, dispatcher <radiance>),
			(v, view)
		),
		LaunchID idx, LaunchSize size
	) {
		r = float3(0);
		r.trace(t, Ray { float3(0), float3(0, 0, 1) }, f32(0.001f), f32(100.0f));
		uint3 pixel = idx;
		img.write(uint2(pixel.xy), float4(r, 0));
	};

	auto chit = $shader(chit)(
		$contracts((r, receiver <radiance>)),
		float2 bary
	) {
		r = float3(bary.x, bary.y, f32(0));
	};

	auto miss = $shader(miss)($contracts((r, receiver <radiance>)))
	{
		r = float3(0);
	};

	auto pipeline = RayTracingCombinator {
		.device = device,
		.compiler = compiler,
	} (rgen, std::tuple { miss }, std::tuple { chit });

	auto cmds = nullptr
		| bind_pipeline(pipeline)
		| bind_descriptors(bound_tlas, bound_image)
		| bind_push_constants <view> (pc)
		| trace_rays(sbt.raygen, sbt.miss, sbt.hit, sbt.callable, 1024, 1024);
}

// Valid: no push constants, just descriptors
void rt_no_push_constants(
	const Device &device, const ShaderCompiler &compiler,
	const BoundDescriptor <tlas> &bound_tlas,
	const BoundDescriptor <image> &bound_image,
	const ShaderBindingTable &sbt
)
{
	auto rgen = $shader(raygen)(
		$contracts(
			(t, tlas), (img, image),
			(r, dispatcher <radiance>)
		),
		LaunchID idx, LaunchSize size
	) {
		r = float3(0);
		r.trace(t, Ray { float3(0), float3(0, 0, 1) }, f32(0.001f), f32(100.0f));
		uint3 pixel = idx;
		img.write(uint2(pixel.xy), float4(r, 0));
	};

	auto chit = $shader(chit)(
		$contracts((r, receiver <radiance>)),
		float2 bary
	) {
		r = float3(bary.x, bary.y, f32(0));
	};

	auto miss = $shader(miss)($contracts((r, receiver <radiance>)))
	{
		r = float3(0);
	};

	auto pipeline = RayTracingCombinator {
		.device = device,
		.compiler = compiler,
	} (rgen, std::tuple { miss }, std::tuple { chit });

	auto cmds = nullptr
		| bind_pipeline(pipeline)
		| bind_descriptors(bound_tlas, bound_image)
		| trace_rays(sbt.raygen, sbt.miss, sbt.hit, sbt.callable, 1024, 1024);
}

// Invalid: trace_rays without binding descriptors
void rt_missing_descriptors(
	const Device &device, const ShaderCompiler &compiler,
	const ResourceTypeFor <view> &pc,
	const ShaderBindingTable &sbt
)
{
	auto rgen = $shader(raygen)(
		$contracts(
			(t, tlas), (img, image),
			(r, dispatcher <radiance>),
			(v, view)
		),
		LaunchID idx, LaunchSize size
	) {
		r = float3(0);
		r.trace(t, Ray { float3(0), float3(0, 0, 1) }, f32(0.001f), f32(100.0f));
		uint3 pixel = idx;
		img.write(uint2(pixel.xy), float4(r, 0));
	};

	auto chit = $shader(chit)(
		$contracts((r, receiver <radiance>)),
		float2 bary
	) {
		r = float3(bary.x, bary.y, f32(0));
	};

	auto miss = $shader(miss)($contracts((r, receiver <radiance>)))
	{
		r = float3(0);
	};

	auto pipeline = RayTracingCombinator {
		.device = device,
		.compiler = compiler,
	} (rgen, std::tuple { miss }, std::tuple { chit });

	auto cmds = nullptr
		| bind_pipeline(pipeline)
		| bind_push_constants <view> (pc)
		| trace_rays(sbt.raygen, sbt.miss, sbt.hit, sbt.callable, 1024, 1024);
}
