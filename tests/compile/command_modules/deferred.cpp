#include <rcgp.hpp>

using namespace rcgp;

namespace gbuf {

struct {
	ColorTarget color;
} targets;

auto pass = $render_pass(writer <targets.color>);

} // namespace gbuf

struct LightingSamplers {
	sampler <gbuf::targets.color> color;
	$reflection(color);
};

inline ResourceGroup <LightingSamplers> lighting_resources;

namespace lighting {

struct {
	ColorTarget screen;
} targets;

auto pass = $render_pass(
	writer <targets.screen>,
	sampled <gbuf::targets.color>
);

} // namespace lighting

// Pass without a sampled<> declaration for gbuf::targets.color, used to
// trigger the unresolved-sample error below.
namespace lighting_missing {

struct {
	ColorTarget screen;
} targets;

auto pass = $render_pass(writer <targets.screen>);

} // namespace lighting_missing

// Valid: gbuffer writes color, lighting declares it as sampled, fragment
// shader reads it via sampler<&t>. SamplesTarget cancels DeclaresSampled.
void deferred_valid(
	const Device &d, const ShaderCompiler &c,
	const BoundDescriptor <lighting_resources> &bound,
	ColorTargetImage *gbuf_color,
	ColorTargetImage *screen)
{
	auto gbuf_vs = $shader(vertex)(ClipPosition clip) { clip = float4(0); };
	auto gbuf_fs = $shader(fragment)() { return float4(1); };

	auto gbuf_pipeline = RasterizationCombinator <Topology::eTriangleList> {
		.device = d, .compiler = c,
		.render_state = gbuf::pass.render_state(),
	} (gbuf_vs, gbuf_fs);

	auto light_vs = $shader(vertex)(ClipPosition clip) { clip = float4(0); };
	auto light_fs = $shader(fragment)(
		$contracts((L, lighting_resources))
	) {
		return L.color.sample(float2(0, 0));
	};

	auto light_pipeline = RasterizationCombinator <Topology::eTriangleList> {
		.device = d, .compiler = c,
		.render_state = lighting::pass.render_state(),
	} (light_vs, light_fs);

	auto gbuf_frame = gbuf::pass.frame(vk::Rect2D())
		.target <gbuf::targets.color> (gbuf_color);

	auto light_frame = lighting::pass.frame(vk::Rect2D())
		.target <lighting::targets.screen> (screen);

	auto cmds = nullptr
		| gbuf_frame.begin()
		| bind_pipeline(gbuf_pipeline)
		| draw(3)
		| gbuf_frame.end()
		| light_frame.begin()
		| bind_pipeline(light_pipeline)
		| bind_descriptors(bound)
		| draw(3)
		| light_frame.end();
}

// Valid: pass declares sampled<> that the shader doesn't use — harmless,
// DeclaresSampled stays as tag_res in the map which the sentinel ignores.
void deferred_overdeclared(
	const Device &d, const ShaderCompiler &c,
	ColorTargetImage *gbuf_color,
	ColorTargetImage *screen)
{
	auto gbuf_vs = $shader(vertex)(ClipPosition clip) { clip = float4(0); };
	auto gbuf_fs = $shader(fragment)() { return float4(1); };

	auto gbuf_pipeline = RasterizationCombinator <Topology::eTriangleList> {
		.device = d, .compiler = c,
		.render_state = gbuf::pass.render_state(),
	} (gbuf_vs, gbuf_fs);

	auto light_vs = $shader(vertex)(ClipPosition clip) { clip = float4(0); };
	auto light_fs = $shader(fragment)() { return float4(1); };

	auto light_pipeline = RasterizationCombinator <Topology::eTriangleList> {
		.device = d, .compiler = c,
		.render_state = lighting::pass.render_state(),
	} (light_vs, light_fs);

	auto gbuf_frame = gbuf::pass.frame(vk::Rect2D())
		.target <gbuf::targets.color> (gbuf_color);

	auto light_frame = lighting::pass.frame(vk::Rect2D())
		.target <lighting::targets.screen> (screen);

	auto cmds = nullptr
		| gbuf_frame.begin()
		| bind_pipeline(gbuf_pipeline)
		| draw(3)
		| gbuf_frame.end()
		| light_frame.begin()
		| bind_pipeline(light_pipeline)
		| draw(3)
		| light_frame.end();
}

// Invalid: fragment shader has sampler<&gbuf::targets.color> but the enclosing
// lighting_missing::pass does not declare sampled<gbuf::targets.color>.
// SamplesTarget stays unresolved on key_sampled_decl, sentinel fires.
void deferred_missing_sampled_decl(
	const Device &d, const ShaderCompiler &c,
	const BoundDescriptor <lighting_resources> &bound,
	ColorTargetImage *gbuf_color,
	ColorTargetImage *screen)
{
	auto gbuf_vs = $shader(vertex)(ClipPosition clip) { clip = float4(0); };
	auto gbuf_fs = $shader(fragment)() { return float4(1); };

	auto gbuf_pipeline = RasterizationCombinator <Topology::eTriangleList> {
		.device = d, .compiler = c,
		.render_state = gbuf::pass.render_state(),
	} (gbuf_vs, gbuf_fs);

	auto light_vs = $shader(vertex)(ClipPosition clip) { clip = float4(0); };
	auto light_fs = $shader(fragment)(
		$contracts((L, lighting_resources))
	) {
		return L.color.sample(float2(0, 0));
	};

	auto light_pipeline = RasterizationCombinator <Topology::eTriangleList> {
		.device = d, .compiler = c,
		.render_state = lighting_missing::pass.render_state(),
	} (light_vs, light_fs);

	auto gbuf_frame = gbuf::pass.frame(vk::Rect2D())
		.target <gbuf::targets.color> (gbuf_color);

	auto light_frame = lighting_missing::pass.frame(vk::Rect2D())
		.target <lighting_missing::targets.screen> (screen);

	auto cmds = nullptr
		| gbuf_frame.begin()
		| bind_pipeline(gbuf_pipeline)
		| draw(3)
		| gbuf_frame.end()
		| light_frame.begin()
		| bind_pipeline(light_pipeline)
		| bind_descriptors(bound)
		| draw(3)
		| light_frame.end();
}
