#include <fmt/printf.h>

#include <ugp.hpp>

#include <mrd.hpp>

struct Vertex {
	vec3 position;
	vec3 normal;
	vec2 uv;

	InstanceIndex instance;

	$reflection(position, normal, uv, instance);
};

struct RasterForward {
	Position svpos;
	Smooth <vec3> position;
	Smooth <vec3> normal;
	Smooth <vec2> uv;

	$reflection(svpos, position, normal, uv);
};

struct FragmentInput {
	vec3 position;
	vec3 normal;
	vec2 uv;

	$reflection(position, normal, uv);
};

struct Camera {
	mat4 view;
	mat4 proj;

	vec4 project(vec4 p) const {
		return proj * view * p;
	}

	$reflection(view, proj);
};

UniformBuffer <Camera> camera;

using Transforms = array <mat4>;

StorageBuffer <Transforms> transforms;

int main()
{
	auto session_info = Session::Info {};
	auto [session, dld] = Session::from(session_info);

	auto device_info = Device::Info {};
	auto device = Device::from(session, dld, device_info);

	// TODO: ensure in compile that ALL resources are bound by reference
	auto vs = $vertex $fn($ref(camera), $ref(transforms), Vertex vertex) -> $returns(RasterForward)
	{
		mat4 xform = transforms[vertex.instance];
		vec4 ppos = xform * vec4(vertex.position, 1);
		vec4 pnorm = xform * vec4(vertex.normal, 0);
		$return RasterForward {
			.svpos = camera.project(ppos),
			.position = vec3(ppos),
			.normal = vec3(pnorm),
			.uv = vertex.uv,
		};
	};

	// TODO: test with one shared and not shared resource... (camera, shading params...)
	auto fs = $fragment $fn(FragmentInput fin) -> $returns(vec4)
	{
		vec3 light = normalize(vec3(1, 1, 1));
		f32 ndotl = max(dot(fin.normal, light), 0.0f);
		$return vec4(vec3(ndotl), 1.0);
	};

	// TODO: should lift to push constant if possible...
	// everything else is degenerated to constant/uniform buffer
	// NOTE: the allocation is kept in the pipeline metadata...
	using allocation = std::tuple <
		group_allocation_record <camera, 0>,
		group_allocation_record <transforms, 1>
	>;

	auto map = new_allocation(allocation());
	vs.apply_group_allocation_map(map);

	auto compiler_info = Compiler::Info {};
	auto compiler = Compiler::from(device, compiler_info);

	auto glsl = generators::GLSL(vs).generate();
	fmt::println("vertex:\n{}", glsl);
	auto spirv = compiler.glsl_to_spirv(glsl, EShLangVertex);
	fmt::println("spirv words: {}", spirv.size());

	// glsl = generators::GLSL(fs).generate();
	// fmt::println("fragment:\n{}", glsl);
	// spirv = compiler.glsl_to_spirv(glsl, EShLangFragment);
	// fmt::println("spirv words: {}", spirv.size());
}
