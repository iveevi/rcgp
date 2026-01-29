#include <rcgp.hpp>

using namespace rcgp;

auto fs = $shader(fragment)(
	WorkGroup <8, 1, 1> group,
	vec4 color
) -> vec4
{
	(void)group;
	return color;
};
