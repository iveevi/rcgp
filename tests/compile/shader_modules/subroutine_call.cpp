#include <rcgp.hpp>

using namespace rcgp;

static Sampler2D texture;

// Valid: direct call on a contract-free subroutine
void direct_call_no_contracts()
{
	auto sr = $subroutine(sr)(f32 x) {
		return float3(x);
	};

	auto vs = $shader(vertex, &)() {
		auto r = sr(1);
		return r;
	};
}

// Invalid: direct call on a contract-bearing subroutine
void direct_call_with_contracts()
{
	auto sr = $subroutine(sr)($contracts((tex, texture)), float2 uv) {
		return tex.sample(uv);
	};

	auto fs = $shader(fragment, &)(
		$contracts((tex, texture)),
		float2 uv
	) -> float4 {
		return sr(uv);
	};
}
