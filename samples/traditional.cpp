#include <array>
#include <concepts>
#include <vector>
#include <glm/glm.hpp>

#include <ugp.hpp>

using x = std430_layout_t <uint32_t, glm::vec3[3], uint32_t>;
static_assert(sizeof(x) == 80);
static_assert(x::offset <0> () == 0);
static_assert(x::offset <1> () == 16);
static_assert(x::offset <2> () == 64);

template <typename T>
struct TupleBuffer {};

template <typename ... Ts>
struct TupleBuffer <trivial_tuple <Ts...>> : Buffer {
	using value_type = trivial_tuple <Ts...>;

	TupleBuffer() = default;

	TupleBuffer(const Buffer &buffer) : Buffer(buffer) {}

	void upload(const value_type &value) {
		Buffer::upload(&value, sizeof(value), 0);
	}

	static TupleBuffer from(const Device &device, vk::BufferUsageFlagBits usage, vk::MemoryPropertyFlagBits properties) {
		TupleBuffer result = Buffer::from(device, sizeof(value_type), usage, properties);
		return result;
	}
};

template <template <typename ...> typename Layout, typename ... Ts>
using TBuffer = TupleBuffer <Layout <Ts...>>;

struct TraditionalGraphicsPipeline : vk::Pipeline {
	vk::PipelineLayout layout;

	struct Info {
		vk::ShaderModule vmodule;
		vk::ShaderModule fmodule;
		vk::RenderPass renderpass;
		vk::Extent2D extent;
		vk::ArrayProxy <const vk::VertexInputBindingDescription> bindings;
		vk::ArrayProxy <const vk::VertexInputAttributeDescription> attributes;
		vk::PipelineLayout layout = {};
	};

	TraditionalGraphicsPipeline() = default;
	TraditionalGraphicsPipeline(vk::Pipeline pipeline, vk::PipelineLayout layout_)
		: vk::Pipeline(pipeline), layout(layout_) {}

	static TraditionalGraphicsPipeline from(const Device &device, const vk::detail::DispatchLoaderDynamic &ldl, const Info &info) {
		auto shader_stages = std::array {
			vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eVertex)
				.setModule(info.vmodule)
				.setPName("main"),
			vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eFragment)
				.setModule(info.fmodule)
				.setPName("main"),
		};

		auto vertex_input = vk::PipelineVertexInputStateCreateInfo()
			.setVertexBindingDescriptions(info.bindings)
			.setVertexAttributeDescriptions(info.attributes);

		auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology(vk::PrimitiveTopology::eTriangleList)
			.setPrimitiveRestartEnable(false);

		auto viewport = vk::Viewport()
			.setX(0.0f)
			.setY(0.0f)
			.setWidth(static_cast <float> (info.extent.width))
			.setHeight(static_cast <float> (info.extent.height))
			.setMinDepth(0.0f)
			.setMaxDepth(1.0f);

		auto scissor = vk::Rect2D()
			.setOffset({ 0, 0 })
			.setExtent(info.extent);

		auto viewport_state = vk::PipelineViewportStateCreateInfo()
			.setViewports(viewport)
			.setScissors(scissor);

		auto rasterization = vk::PipelineRasterizationStateCreateInfo()
			.setDepthClampEnable(false)
			.setRasterizerDiscardEnable(false)
			.setPolygonMode(vk::PolygonMode::eFill)
			.setCullMode(vk::CullModeFlagBits::eBack)
			.setFrontFace(vk::FrontFace::eClockwise)
			.setDepthBiasEnable(false)
			.setLineWidth(1.0f);

		auto multisampling = vk::PipelineMultisampleStateCreateInfo()
			.setRasterizationSamples(vk::SampleCountFlagBits::e1)
			.setSampleShadingEnable(false)
			.setMinSampleShading(1.0f);

		auto color_blend_attachment = vk::PipelineColorBlendAttachmentState()
			.setBlendEnable(false)
			.setColorWriteMask(
				vk::ColorComponentFlagBits::eR |
				vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB |
				vk::ColorComponentFlagBits::eA
			);

		auto color_blend = vk::PipelineColorBlendStateCreateInfo()
			.setAttachments(color_blend_attachment);

		auto pipeline_layout = info.layout;
		if (!pipeline_layout)
			pipeline_layout = device.logical.createPipelineLayout(vk::PipelineLayoutCreateInfo(), nullptr, ldl);

		auto pipeline_info = vk::GraphicsPipelineCreateInfo()
			.setLayout(pipeline_layout)
			.setPInputAssemblyState(&input_assembly)
			.setPVertexInputState(&vertex_input)
			.setPRasterizationState(&rasterization)
			.setPMultisampleState(&multisampling)
			.setPColorBlendState(&color_blend)
			.setPViewportState(&viewport_state)
			.setStages(shader_stages)
			.setRenderPass(info.renderpass)
			.setSubpass(0);

		auto [result, pipeline] = device.logical.createGraphicsPipeline(nullptr, pipeline_info, nullptr, ldl);
		// (void)result; // TODO: handle failures gracefully

		return TraditionalGraphicsPipeline(pipeline, pipeline_layout);
	}
};

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
	// TODO: what if we encode the vertices? then jit vec and struct vec
	// should be different... but we can still treat the encoded vec
	// as a vec2 in DSL code...
	// Or, better yet, allow the user to define their own derivative
	// which is then translated in different ways...
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

	using PBuffer = TBuffer <std430_layout_t, uint32_t, glm::vec3[3], uint32_t>;

	// auto pbuf = PBuffer::from(
	// 	device,
	// 	vk::BufferUsageFlagBits::eUniformBuffer,
	// 	vk::MemoryPropertyFlagBits::eDeviceLocal
	// );
	//
	// auto pbuf_value = PBuffer::value_type();
	// pbuf_value.get <0> () = 12;
	// pbuf_value.get <1> ()[2] = glm::vec3(1);
	//
	// pbuf.upload(pbuf_value);

	// TODO: should be TBuffer <std430..., glm::vec2[]>
	auto vertices = std::array {
		glm::vec2(-0.5f, -0.5f),
		glm::vec2(0.5f, -0.5f),
		glm::vec2(0.0f, 0.5f),
	};

	auto vertex_buffer = Buffer::from(
		device,
		sizeof(vertices),
		vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
	);

	vertex_buffer.upload(vertices.data(), sizeof(vertices));

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
		cmd.bindVertexBuffers(0, vertex_buffer.handle, vb_offset);
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
