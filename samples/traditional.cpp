#include <array>
#include <vector>
#include <glm/glm.hpp>

#include <ugp.hpp>

const std::string vshader = R"(
#version 450

layout (location = 0) in vec2 position;

void main()
{
	gl_Position = vec4(position, 0, 1);
}
)";

const std::string fshader = R"(
#version 450

layout (location = 0) out vec4 color;

void main()
{
	color = vec4(1);
}
)";

namespace detail {

// TODO: custom implementation later?
template <typename T>
using unsized_array = std::vector <T>;

} // namespace detail

namespace std430 {

template <typename T>
struct layout_engine {
	static_assert(false, ($ss("s::layout_engine not implemented for type ") + $ss_type(T)).view());
};

template <typename Original, typename ... Ts>
struct layout_engine <aggregate_reflection <Original, Ts...>> {
	static constexpr size_t alignment = std::max({ layout_engine <Ts> ::alignment... });
	using hint = scaffold_hint <
		sequence <typename layout_engine <Ts> ::hint...>,
		alignment
	>;
};

template <typename T, int64_t N>
requires (N > 0)
struct layout_engine <array_reflection <T, N>> {
	static constexpr size_t alignment = layout_engine <T> ::alignment;
	using hint = scaffold_hint <
		std::array <typename layout_engine <T> ::hint, N>,
		alignment
	>;
};

template <native_scalar T, size_t N, size_t M>
struct layout_engine <primitive_reflection <matrix <T, N, M>>> {
	// TODO: equals the alignment of row vector
	static constexpr size_t alignment = 16;
	using hint = scaffold_hint <
		glm::mat <M, N, T>,
		alignment
	>;
};

template <native_scalar T, size_t D>
struct layout_engine <primitive_reflection <vector <T, D>>> {
	static constexpr size_t alignment = [] constexpr {
		switch (D) {
		case 4:
		case 3:
			return 16;
		case 2:
			return 8;
		}
	} ();

	using hint = scaffold_hint <
		glm::vec <D, T>,
		alignment
	>;
};

} // namespace std430

namespace layouts {

template <typename T>
using std430 = std430::layout_engine <T>;

} // namespace layouts

template <typename T, template <typename> typename Engine = layouts::std430>
struct data_mirror {
	using layout = Engine <expand_reflection_t <T>>;
	using hint = layout::hint;
	// TODO: remove
	using lookup = scaffold_lookup <hint, T>;
	using type = scaffold_lookup <hint, T> ::type;
};

#define $mirror(T, ...) data_mirror <T __VA_OPT__(,) __VA_ARGS__> ::type

struct View {
	mat4 model;
	mat4 view;
	mat4 proj;

	$reflection(model, view, proj);
};

struct X {
	vec3 x;
	vec3 y;

	$reflection(x, y);
};

struct XNested {
	vec2 w;
	X nested;

	$reflection(w, nested);
};

auto ftn = [] {
	using Y = $mirror(X);

	static_assert(sizeof(Y) == 32);
	static_assert(offsetof(Y, x) == 0);
	static_assert(offsetof(Y, y) == 16);

	Y y {
		.x = glm::vec3(1.0f),
		.y = glm::vec3(2.0f),
	};

	auto s = $mirror(View) {
		.model = glm::mat4(1.0),
		.proj = glm::mat4(0.1),
	};

	using Z = $mirror(array <vec2, 3>);

	Z z;
	z[2] = glm::vec2(2.0f);

	using A = $mirror(XNested);

	A a {
		.nested = {
			.y = glm::vec3(1.0f),
		},
	};

	a.nested.x = glm::vec3(1.0f);
	
	using W = $mirror(array <X, 12>);
	W w {
		W::value_type {
			.x = glm::vec3(1.0f),
		},
	};
};

// NOTE: scaffolds will be used for mirrors:
// - data mirrors use $mirror(X) and simply translate the
// 	aggregate into a sequence and apply layout engines to
// 	get the final tuple type (dynamic or trivial)
// - resource mirrors obtained from pipeline handles (with .descriptor <ref>)
// 	will hold a pseudo-descriptor which can be assigned various
// 	resources (i.e. ggx_handle.set(ggx_buffer)); for parameters
// 	block we may need to split into a constant and dynamic part?
// 	OR we can require the free fields (not in buffers) to be constant/ordinary...
// 	(this is the preferred approach)...
// 	then each pblock_handle has a constant parts from ordinary fields
// 	and pseudo-descriptors for the rest; but should we explicitly manage
// 	the constant part?? I think they should all point to the same pseudo-descriptor...
// 	SOLN: if a field is in teh constant block then make its constant_block_shadow
// 	and then provide a constant field which is its own pseudo-desciptor for the whole cblock
// 	the general philosophy here is: dont manage data for the user, just provide a scaffold
// 	for the resources and binding...
// 	as a corellary, parameter block handles are trivial tuples (scaffolds over)
// 	then we also need a $constant(pblock) mirror which extracts a trviial_tuple from the
// 	parameter block and sets all resource fields to constant_block_disable
// 	UNLESS there is some way to mask out fields...

// TODO: write down the implementation design somewhere in docs...
// will help spotting parts that are stupid
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
	// auto &ldev = device.logical;

	auto window = Window::from(session, device);

	auto queue = Queue::from(device);
	auto cpool = CommandPool::from(device, queue);
	auto cmdbuffers = group(device, cpool).allocate(window.frames_in_flight);

	auto compiler = Compiler::from(device, Compiler::Info());
	
	auto vspv = compiler.glsl_to_spirv(vshader, EShLangVertex);
	auto fspv = compiler.glsl_to_spirv(fshader, EShLangFragment);

	auto vmodule = compiler.spirv_to_shader_module(vspv);
	auto fmodule = compiler.spirv_to_shader_module(fspv);

	// TODO: translate a sequence of types to binding AND attributes
	// 
	// TODO: what if we encode the vertices? then jit vec and struct vec
	// should be different... but we can still treat the encoded vec
	// as a vec2 in DSL code...
	// Or, better yet, allow the user to define their own derivative
	// which is then translated in different ways...
	//
	// TODO: actually vertices also have a layout (default would be scalar)
	// but that still doesnt fix the encoding shit
	auto binding_descs = std::array {
		vk::VertexInputBindingDescription()
			.setBinding(0)
			.setStride(sizeof(glm::vec2))
			.setInputRate(vk::VertexInputRate::eVertex),
	};

	auto attribute_descs = std::array {
		vk::VertexInputAttributeDescription()
			.setLocation(0)
			.setBinding(0)
			.setFormat(vk::Format::eR32G32Sfloat)
			.setOffset(0),
	};

	auto attachments = Attachments();

	attachments["color"] = vk::AttachmentDescription()
		.setFormat(window.format)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
		.setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);

	auto color = attachments.reference("color", vk::ImageLayout::eColorAttachmentOptimal);

	// TODO: could make this a group(device, dld) method?
	auto rp = renderpass(device, dld,
		attachments,
		subpass(color_attachments { color },
			depth_attachments {},
			color_attachments {},
			input_attachments {})
	);

	std::vector <vk::Framebuffer> framebuffers;
	framebuffers.reserve(window.images.size());
	for (auto &image : window.images) {
		auto fb_info = vk::FramebufferCreateInfo()
			.setRenderPass(rp)
			.setAttachments(image.view)
			.setWidth(image.extent.width)
			.setHeight(image.extent.height)
			.setLayers(1);

		framebuffers.push_back(device.logical.createFramebuffer(fb_info, nullptr, dld));
	}

	auto pipeline_info = TraditionalGraphicsPipeline::Info {
		.vmodule = vmodule,
		.fmodule = fmodule,
		.renderpass = rp,
		.extent = window.extent(),
		.bindings = binding_descs,
		.attributes = attribute_descs,
	};

	auto pipeline = TraditionalGraphicsPipeline::from(device, dld, pipeline_info);

	auto vbuf = Buffer::from(
		device,
		3 * sizeof(glm::vec2),
		vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	);

	auto dyn = std::vector <glm::vec2> ();
	dyn.resize(3);

	dyn[0] = glm::vec2(-0.5f, -0.5f);
	dyn[1] = glm::vec2(0.5f, -0.5f);
	dyn[2] = glm::vec2(0.0f, 0.5f);

	vbuf.upload(dyn.data(), dyn.size() * sizeof(glm::vec2));

	while (window.alive()) {
		window.poll();

		if (glfwGetKey(window.handle, GLFW_KEY_Q) == GLFW_PRESS)
			glfwSetWindowShouldClose(window.handle, true);

		auto frame = window.next_frame();

		group(device, window).wait(frame);
		auto acquired = group(device, window).acquire_image(frame);
		if (!acquired)
			continue;

		auto &cmd = cmdbuffers[window.frame_index];

		cmd.reset();
		cmd.begin(vk::CommandBufferBeginInfo());

		auto &image = window.image(frame.image_index);
		auto to_color = image.memory_barrier(
			vk::ImageLayout::eColorAttachmentOptimal,
			{},
			vk::AccessFlagBits::eColorAttachmentWrite
		);

		cmd.pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			{}, {}, {}, to_color
		);

		auto render_area = vk::Rect2D()
			.setOffset({ 0, 0 })
			.setExtent(frame.extent);

		auto clear_value = vk::ClearValue()
			.setColor(vk::ClearColorValue(std::array <float, 4> { 0.05f, 0.05f, 0.05f, 1.0f }));

		auto rp_begin = vk::RenderPassBeginInfo()
			.setRenderPass(rp)
			.setFramebuffer(framebuffers[frame.image_index])
			.setRenderArea(render_area)
			.setClearValues(clear_value);

		cmd.beginRenderPass(rp_begin, vk::SubpassContents::eInline);
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
		vk::DeviceSize vb_offset = 0;
		cmd.bindVertexBuffers(0, vbuf.handle, vb_offset);
		cmd.draw(3, 1, 0, 0);
		cmd.endRenderPass();

		auto to_present = image.memory_barrier(
			vk::ImageLayout::ePresentSrcKHR,
			vk::AccessFlagBits::eColorAttachmentWrite,
			{}
		);

		cmd.pipelineBarrier(
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::PipelineStageFlagBits::eBottomOfPipe,
			{}, {}, {}, to_present
		);

		cmd.end();

		vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;

		auto submit_info = vk::SubmitInfo()
			.setCommandBuffers(cmd)
			.setSignalSemaphores(frame.rendered)
			.setWaitSemaphores(frame.presented)
			.setWaitDstStageMask(wait_stage);

		queue.submit(submit_info, frame.fence);

		auto present_info = vk::PresentInfoKHR()
			.setImageIndices(frame.image_index)
			.setSwapchains(window.swapchain)
			.setWaitSemaphores(frame.rendered);

		auto result = queue.presentKHR(present_info);
	}
}
