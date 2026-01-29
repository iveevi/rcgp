#include <rcgp.hpp>

using namespace rcgp;

AttributeStream <vec3> position;

auto vs = $shader(vertex)(
	$contracts(position),
	ClipPosition clip_position
) -> std::tuple <Smooth <vec3>, Smooth <vec3>>
{
	clip_position = vec4(position, 1.0f);
	return std::tuple(position, position);
};

auto fs = $shader(fragment)(
	vec3 color
) -> vec4
{
	return vec4(color, 1.0f);
};

extern Device device;
extern ShaderCompiler compiler;
extern vk::RenderPass render_pass;
extern vk::Extent2D extent;

[[maybe_unused]] void compile_mismatch()
{
	RasterizationOptions options{extent, false};
	RasterizationCombinator <Topology::eTriangleList> comb{
		device,
		compiler,
		render_pass,
		options
	};
	comb(vs, fs);
}
