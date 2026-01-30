#include <utility>

#include <rcgp.hpp>

using namespace rcgp;

auto vs = $shader(vertex)(
	ClipPosition clip_position
) -> Smooth <vec4>
{
	clip_position = vec4(0.0f);
	return Smooth <vec4> (vec4(0.0f));
};

auto fs = $shader(fragment)(
	vec4 color
) -> vec4
{
	return color;
};

// TODO: $rasteration_pipeline_of(T, x, y)
using Pipeline = decltype(std::declval <RasterizationCombinator <Topology::eTriangleList> &> ()(vs, fs));

extern Pipeline pipeline;

[[maybe_unused]] void record_missing_index()
{
	auto cmds = bind_pipeline(pipeline) | draw_indexed(3);
	(void)cmds;
}
