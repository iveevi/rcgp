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

struct alignas(0) Points {
	u32 count;

	struct Array {
		vec3 delta;

		// array <u32> positions;
		array <vec2> positions;

		$reflection(delta, positions);
	} array;

	$reflection(count, array);
	// TODO: if T is a dynamic aggregate,
	// then align should fallback to 0 (i.e. natural)
};

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

	// TODO: cube rendering...
	auto vbuf = MirrorBuffer <array <vec2>> ::from(
		device,
		12,
		vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	);

	auto vbuf_value = vbuf.new_value();

	vbuf_value.resize(3);
	vbuf_value[0] = glm::vec2(-0.5f, -0.5f);
	vbuf_value[1] = glm::vec2(0.5f, -0.5f);
	vbuf_value[2] = glm::vec2(0.0f, 0.5f);

	vbuf.write(vbuf_value);

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
