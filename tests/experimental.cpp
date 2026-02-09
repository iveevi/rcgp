#include <rcgp.hpp>

using namespace rcgp;

struct Textures {
	Sampler2D albedo;
	Sampler2D specular;

	$reflection(albedo, specular);
};

// TODO: test nested...

// TODO: compile time test with static assertions for field types
static ResourceGroup <Textures> textures;

static_assert(std::is_same_v <decltype(contract <textures> ().albedo), Sampler2D::handle_type>);
static_assert(std::is_same_v <decltype(contract <textures> ().specular), Sampler2D::handle_type>);

auto fs = $shader(fragment)($contracts(textures))
{
	return textures.albedo.sample(float2(0, 0));
};

static AttributeStream <float3> vertices;

auto vs = $shader(vertex)($contracts(vertices))
{
};

// TODO: compilation assertion tests...
using host_textures = ResourceTypeFor <textures>;

// TODO: analyze compile time incrementally... it already really slow
int main()
{
	// auto glsl = generate_glsl(fs);
	auto glsl = generate_glsl(vs);
	printf("glsl:\n%s\n", glsl.c_str());
}
