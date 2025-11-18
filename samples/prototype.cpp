#include <fmt/printf.h>

#include <ugp.hpp>

struct Vertex {
	vec3 position;
	vec3 normal;
	vec2 uv;

	instance_index_t instance;

	$reflection(position, normal, uv, instance);
};

struct RasterForward {
	Position svpos;
	Smooth <vec3> position;
	Smooth <vec3> normal;
	Smooth <vec2> uv;

	$reflection(svpos, position, normal, uv);
};

struct Camera {
	mat4 view;
	mat4 proj;

	vec4 project(vec4 p) const {
		return proj * view * p;
	}

	$reflection(view, proj);
};

ParameterBlock <Camera> camera;

using Transforms = array <mat4>;

StructuredBuffer <Transforms> transforms;

int main()
{
	auto vs = $vertex $fn($use(camera), $use(transforms), Vertex vertex) -> $returns(RasterForward)
	{
		auto xform = transforms[1];
		auto ppos = xform * vec4(vertex.position, 1);
		auto pnorm = xform * vec4(vertex.normal, 0);
		$return RasterForward {
			.svpos = camera.project(ppos),
			.position = vec3(ppos),
			.normal = vec3(pnorm),
			.uv = vertex.uv,
		};
	};

	fmt::println("assembly:");
	fmt::println("{}", generators::Assembly(vs).generate());

	// fmt::println("glsl:");
	// fmt::println("{}", generators::GLSL(vs).generate());
}
