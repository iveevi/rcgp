#include <ugp.hpp>

namespace shaders {

struct Vertex {
	vec3 position;
	vec3 normal;
	vec2 uv;

	$reflection(position, normal, uv);
};

struct Model {
	mat4 xform;
	i32 material;

	$reflection(xform, material);
};

struct Camera {
	mat4 view;
	mat4 proj;

	vec4 project(vec4 p) {
		return proj * view * p;
	}

	$reflection(view, proj);
};

struct RasterForward {
	Position svpos;
	Smooth <vec3> position;
	Smooth <vec3> normal;
	Smooth <vec2> uv;
	Flat <i32> material;

	$reflection(svpos, position, normal, uv, material);
};

struct GBuffer {
	vec3 position;
	vec3 normal;
	vec2 uv;
	i32 material;
};

ParameterBlock <Camera> camera;
ParameterBlock <Model> model;

auto gbuffer_vs = $vertex $fn($ref(camera), $ref(model), Vertex vertex) -> $returns(RasterForward)
{
	auto ppos = model.xform * vec4(vertex.position, 1.0);
	auto pnorm = model.xform * vec4(vertex.normal, 0.0);
	$return RasterForward {
		.svpos = camera.project(ppos),
		.position = ppos.xyz,
		.normal = pnorm.xyz,
		.uv = vertex.uv,
		.material = model.material,
	};
};

auto gbuffer_fs = $fragment $fn($ref(model), RasterForward rfwd) -> $returns(GBuffer) {
	$return GBuffer {
		.position = rfwd.position,
		.normal = rfwd.normal,
		.uv = rfwd.uv,
		.material = rfwd.material,
	};
};

struct Material {
	Sampler2D diffuse;
	Sampler2D specular;
	Sampler2D roughness;

	std::tuple <vec3, vec3, f32> sample(vec2 uv) const {
		vec3 d = diffuse.sample(uv);
		vec3 s = specular.sample(uv);
		f32 r = roughness.sample(uv);
		return { d, s, r };
	}

	$reflection(diffuse, specular, roughness);
};

struct ShadingParameters {
	f32 gamma;
	vec3 tint;
	array <Material> materials;
};

ParameterBlock <ShadingParameters> shading_parameters;

struct SurfaceInfo {
	vec3 position;
	vec3 normal;
};

// TODO: constraint?
struct Phong {
	vec3 diffuse;
	vec3 specular;
	f32 roughness;

	vec3 shade(SurfaceInfo sinfo) {
		// ...
		return vec3();
	}
};

// TODO: template by material...
// TODO: forward shading modularity
auto shading_fs = $fragment $fn($ref(shading_parameters), SubpassInput <GBuffer> gbuf) -> $returns(vec4)
{
	auto material = shading_parameters.materials[gbuf.material];
	auto [d, s, r] = material.sample(gbuf.uv);
	auto brdf = Phong(d, s, r);
	auto sinfo = SurfaceInfo(gbuf.position, gbuf.normal);
	auto color = brdf.shade(sinfo);
	auto tmap = pow(color, vec3(1.0 / shading_parameters.gamma));
	$return vec4(tmap, 1.0);
};

} // namespace shaders

struct GeometryBuffer {
	using vertex = $solid(shaders::Vertex);

	TBuffer <glm::ivec3> triangles;
	TBuffer <vertex> vertices;
};

int main()
{
	auto session_info = Session::Info {
		.validation_bootstrap = false,
	};

	auto [session, dld] = Session::from(session_info);

	auto device_info = Device::Info {
		.extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		},
	};

	auto device = Device::from(session, dld, device_info);
	auto &ldev = device.logical;

	auto window = Window::from(session, device);

	auto compiler = Compiler::from(device, Compiler::Info());
	
	auto gvsm = compiler(shaders::gbuffer_vs);
	auto gfsm = compiler(shaders::gbuffer_fs);
	auto sfsm = compiler(shaders::shading_fs <GGX>);

	auto renderpass = // ...

	auto tgpc = combinators::tgp <Triangle> (renderpass, Culling::None);
	auto ppl = tpgc(gvsm, gfsm, sfsm(13));
	// auto ppl = tpgc(gvsm, sfsm);

	// auto mvp_handle = ppl.handle <mvp> ();
	auto handle = ppl.handle <model> ();

	auto queue = Queue::from(device);
	auto cpool = CommandPool::from(device, queue);
	auto cmdbuffers = group(device, cpool).allocate(window.frames_in_flight);

	// Work[Neutral, Neutral] := Cmd[Neutral] -> Cmd[Neutral]
	auto render = [&](const std::vector <GeometryBuffer> &geobufs) {
		// Cmd[X] -> Cmd[Y]
		auto a = begin_render_pass(...);
		auto b = bind_pipeline(...);

		// b: Work[RP, (PPL, model, camera, ...)]
		for (auto &geobuf : geobufs) {
			auto mvp = ...;
			mvp_handle.write(mvp);
			auto c = bind_resource(mvp_handle);
			// seq(b, c) : Work[RP, (PPL, ..., vb)]
			auto d = bind_vertex_buffer(...);
			// seq(b, c, d) : Work[RP, PPL]
			auto e = draw_indexed(...);
			// seq(b, c, d, e) : Work[RP, RP]
			b |= seq(c, d, e); // b = seq(b, c, d, e);
		}

		auto f = end_render_pass(...);

		return seq(a, b, f);
	};

	auto model = Model::load("resources/meshes/armadillo.obj");
	auto geobufs = model.map([&](auto &mesh)
	{
		return GeometryBuffer::from(device, mesh);
	});

	while (window.alive()) {
		// ...
		queue.submit(render(geobufs), ...);
		// ...
	}
}
