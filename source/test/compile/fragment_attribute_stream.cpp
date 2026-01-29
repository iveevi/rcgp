#include <rcgp.hpp>

using namespace rcgp;

AttributeStream <vec3> position;

auto fs = $shader(fragment)(
	$contracts(position),
	vec4 color
) -> vec4
{
	return color;
};
