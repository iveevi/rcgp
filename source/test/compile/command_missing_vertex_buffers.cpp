#include <utility>

#include <rcgp.hpp>

using namespace rcgp;

AttributeStream <vec3> position;

auto vs = $shader(vertex)(
	$contracts(position),
	ClipPosition clip_position
) -> Smooth <vec4>
{
	clip_position = vec4(position, 1.0f);
	return Smooth <vec4> (vec4(position, 1.0f));
};

auto fs = $shader(fragment)(
	vec4 color
) -> vec4
{
	return color;
};

using Pipeline = decltype(std::declval <RasterizationCombinator <Topology::eTriangleList> &> ()(vs, fs));

extern Pipeline pipeline;

using TriangleBuffer = IndexBuffer <Topology::eTriangleList, uint32_t>;
extern TriangleBuffer ibuffer;

[[maybe_unused]] void record_missing_vertex()
{
	auto cmds = bind_pipeline(pipeline)
		| bind_index_buffer <Topology::eTriangleList> (ibuffer)
		| draw_indexed(3);
	(void)cmds;
}
