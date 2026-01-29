#include <rcgp.hpp>

using namespace rcgp;

auto fs = $shader(fragment)(
	TaskPayload <vec4> payload,
	vec4 color
) -> vec4
{
	(void)payload;
	return color;
};
