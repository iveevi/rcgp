#include "common.hpp"

#define SUITE "resolves"

add_test(vs_fs_io_rates)
{
	auto vs = $shader(vertex)() {
		return std::tuple {
			Smooth <f32> { 1 },
			NoPerspective <float2> { float2(1) },
			Flat <uint4> { uint4(1) },
		};
	};
	
	auto fs = $shader(fragment)(f32 x, float2 y, uint4 z) {
		return float3(1);
	};

	transfer_io_rates(vs, fs);

	assert_glsl_match(fs, R"(
	#version 460

#extension GL_EXT_scalar_block_layout : require
	
	layout (location = 0) smooth in float lin0;
	layout (location = 1) noperspective in vec2 lin1;
	layout (location = 2) flat in uvec4 lin2;
	
	layout (location = 0) out vec3 lout0;
	
	void main()
	{
	    float lvar0;
	    lvar0 = 1;
	    vec3 lvar1;
	    lvar1 = vec3(lvar0, lvar0, lvar0);
	    lout0 = lvar1;
	}
	)");
};
