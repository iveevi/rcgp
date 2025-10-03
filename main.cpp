#include <fmt/printf.h>

#include "ugp.hpp"

struct VertexAttributes {
	Topology <vec3> ::Triangle position;
	Topology <vec3> ::Triangle normal;
	Topology <vec2> ::Triangle uv;

	// TODO: intrinscs must be listed as members... (or parameters...)

	$reflection(position, normal, uv);
};

struct RasterForward {
	Position svpos;
	Interpolant <vec3> ::Smooth position;
	Interpolant <vec3> ::Smooth normal;
	Interpolant <vec2> ::Smooth uv;

	$reflection(svpos, position, normal, uv);
};

// RasterForward == FragmentAttributes after discarding Position
// and intrinscs on both sides and including order
struct FragmentAttributes {
	vec3 position;
	vec3 normal;
	vec2 uv;

	// NOTE: Interpolation qualifiers are inferred (should not necessary be re-declared here...)

	$reflection(position, normal, uv);
};

struct MVP {
	mat4 model;
	mat4 view;
	mat4 proj;

	$reflection(model, view, proj);
};

ParameterBlock <MVP> mvp;

auto vshader = $def(RasterForward)($import(mvp), VertexAttributes va)
{
	// vec4 hpos = vec4(va.position, 1);
	// vec4 mpos = mvp.model * hpos;
	// $return RasterForward {
	// 	// TODO: trace source location to debug errors
	// 	.svpos = mvp.proj * (mvp.view * mpos),
	// 	.position = mpos.xyz(),
	// 	.normal = transpose(inverse(mat3(mvp.model))) * va.normal,
	// 	.uv = va.uv,
	// };
};

template <int P>
auto vshader_generator(float scale)
{
	return $scoped_def(RasterForward)($import(mvp), VertexAttributes va)
	{
		// // TODO: this needs to be piped into a jit begin/end scope...
		// vec4 hpos = vec4(va.position, 1);
		// vec4 mpos = mvp.model * hpos;
		// $return RasterForward {
		// 	.svpos = mvp.proj * (mvp.view * mpos),
		// 	.position = mpos.xyz(),
		// 	.normal = transpose(inverse(mat3(mvp.model))) * va.normal,
		// 	.uv = va.uv,
		// };
	};
}

struct Sampler2D {
	vec4 sample(vec2 uv) {
		// return vec4(1, 0, 0, 1);
	}

	// TODO: disable copies, etc. unique placeholder ID per instance...
};

struct PBRTextures {
	Sampler2D diffuse;
	Sampler2D specular;
	
	$reflection(diffuse, specular);
};

static_assert(reflection_holder <PBRTextures>);
static_assert(std::is_same_v <PBRTextures::This, PBRTextures>);

// TODO: index overwrite using layout... same for vertex input attributes...
ParameterBlock <PBRTextures> textures;

auto fshader = $def(SubpassResult <vec4>)($import(textures), $import(mvp), FragmentAttributes fa)
{
	// vec3 normal = fa.normal;
	// vec2 uv = fa.uv;
	// vec3 color = textures.diffuse.sample(uv).xyz();
	// $return vec4(color, 1);
};

int main()
{
	using A = decltype(vshader)::reflection;
	using B = VertexAttributes::reflection;
	using C = reflection_fetcher <VertexAttributes> ::type;

	static_assert(reflection_holder <ParameterBlock <MVP>>);
	using P = decltype(mvp)::reflection;

	using D = decltype(fshader)::reflection;

	fmt::println("mvp type: {}", $ss_type(decltype(mvp)).view());

	fmt::println("reflected signature of vshader:\n{}", $ss_type(A).view());
	fmt::println("reflected signature of fshader:\n{}", $ss_type(decltype(fshader)::reflection).view());

	using R1 = resource_reference_t <mvp>;
	using R2 = resource_reference_t <mvp>;
	using R3 = resource_reference_t <textures>;

	// TODO: static_reference as a basic unit for testing reference identity
	// static_assert(std::is_same_v <R1, R2>);
	// static_assert(std::is_same_v <R1, R3>,
	//        $ss_format(
	// 		$ss("see the following diagnostic from Javelin:\n\n"
	//        			"\tJavelin: blah blah blah: {}\n"),
	//        		$ss_type_indented(VertexAttributes::reflection, 1)
	//    	).view()
	// );
}
