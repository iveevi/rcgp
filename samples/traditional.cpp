#include <array>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ugp.hpp>

#include "util/aperature.hpp"
#include "util/transform.hpp"

// struct alignas(0) Points {
// 	u32 count;
//
// 	struct Array {
// 		vec3 delta;
//
// 		// array <u32> positions;
// 		array <vec2> positions;
//
// 		$reflection(delta, positions);
// 	} array;
//
// 	$reflection(count, array);
// 	// TODO: if T is a dynamic aggregate,
// 	// then align should fallback to 0 (i.e. natural)
// 	// for it to work would be need to pack structs with pragma?
// };

// TODO: StructuredBuffer becomes:
// - StorageBuffer <T>
// - ArrayBuffer <T> := StorageBuffer <array <T>>
//
// it shall be a pure handle, and then references
// must be with ParameterBlock <StorageBuffer <T>>
// with an alias as StorageBufferResource

template <typename T, vk::VertexInputRate>
struct attribute_stream_reflection {};

// Standalone resource, so no qualms about being embedded within parameter blocks
template <reflected T, vk::VertexInputRate R = vk::VertexInputRate::eVertex>
struct AttributeStream {
	using reflection = attribute_stream_reflection <T, R>;
	DEFINE_REFLECTION_STAMP();
};

template <reflected T, vk::VertexInputRate R, AttributeStream <T, R> &rsrc>
struct reference_base <rsrc> {
	using type = T;
};

template <reflected T, vk::VertexInputRate R, AttributeStream <T, R> &rsrc>
struct stage_argument_injector <Stage::RepresentationalVertex, reference <rsrc>> {
	static auto main(reference <rsrc> &value, const InjectionState &state) {
		return stage_argument_injector <Stage::RepresentationalVertex, T> ::main(value, state);
	}
};

struct View {
	mat4 proj;
	mat4 view;
	mat4 model;

	$reflection(proj, view, model);
};

AttributeStream <vec3> position;
AttributeStream <vec3> color;

MonoConstantBuffer <View> view;

auto vs = $vertex $fn($use(position), $use(color), $use(view)) -> $returns(Position, vec3)
{
	auto mvp = view.proj * view.view * view.model;
	$return std::tuple(Position(mvp * vec4(position, 1.0)), color);
};

auto fs = $fragment $fn(vec3 color) -> $returns(vec4)
{
	$return vec4(color, 1);
};

// TODO: structs need a layout (defaults to std430)

// TODO: write down the implementation design somewhere in docs...
// will help spotting parts that are stupid
int main()
{
	auto session_info = Session::Info {
		// .validation = false,
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
	
	using allocation = std::tuple <
		group_allocation_record <view, 0>
	>;

	auto map = new_allocation(allocation());
	vs.apply_group_allocation_map(map);
	fs.apply_group_allocation_map(map);

	auto vshader = generators::GLSL(vs).generate();
	auto fshader = generators::GLSL(fs).generate();

	fmt::println("vertex shader:\n{}", vshader);
	fmt::println("fragment shader:\n{}", fshader);
	
	auto vspv = compiler.glsl_to_spirv(vshader, EShLangVertex);
	auto fspv = compiler.glsl_to_spirv(fshader, EShLangFragment);

	auto vmodule = compiler.spirv_to_shader_module(vspv);
	auto fmodule = compiler.spirv_to_shader_module(fspv);

	auto binding_descs = std::array {
		vk::VertexInputBindingDescription()
			.setBinding(0)
			.setStride(sizeof(glm::vec4))
			.setInputRate(vk::VertexInputRate::eVertex),
		vk::VertexInputBindingDescription()
			.setBinding(1)
			.setStride(sizeof(glm::vec4))
			.setInputRate(vk::VertexInputRate::eVertex),
	};

	auto attribute_descs = std::array {
		vk::VertexInputAttributeDescription()
			.setLocation(0)
			.setBinding(0)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(0),
		vk::VertexInputAttributeDescription()
			.setLocation(1)
			.setBinding(1)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(0),
	};

	auto set_layout_binding = vk::DescriptorSetLayoutBinding()
		.setBinding(0)
		.setDescriptorCount(1)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setStageFlags(vk::ShaderStageFlagBits::eVertex);

	auto descriptor_set_layout = device.logical.createDescriptorSetLayout(
		vk::DescriptorSetLayoutCreateInfo().setBindings(set_layout_binding),
		nullptr,
		dld
	);

	auto pipeline_layout = device.logical.createPipelineLayout(
		vk::PipelineLayoutCreateInfo().setSetLayouts(descriptor_set_layout),
		nullptr,
		dld
	);

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
		// TODO: wrapper... group(device, rp).new_framebuffer(image, 1)?
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
		.layout = pipeline_layout,
	};

	auto pipeline = TraditionalGraphicsPipeline::from(device, dld, pipeline_info);

	// Positions
	const std::array positions = {
		// Front (+Z)
		glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec3( 1.0f, -1.0f,  1.0f), glm::vec3( 1.0f,  1.0f,  1.0f),
		glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec3(-1.0f,  1.0f,  1.0f),
		// Back (-Z)
		glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3( 1.0f,  1.0f, -1.0f),
		glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3( 1.0f,  1.0f, -1.0f), glm::vec3( 1.0f, -1.0f, -1.0f),
		// Left (-X)
		glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec3(-1.0f,  1.0f,  1.0f),
		glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec3(-1.0f,  1.0f, -1.0f),
		// Right (+X)
		glm::vec3( 1.0f, -1.0f, -1.0f), glm::vec3( 1.0f,  1.0f, -1.0f), glm::vec3( 1.0f,  1.0f,  1.0f),
		glm::vec3( 1.0f, -1.0f, -1.0f), glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec3( 1.0f, -1.0f,  1.0f),
		// Top (+Y)
		glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec3( 1.0f,  1.0f,  1.0f),
		glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3( 1.0f,  1.0f,  1.0f), glm::vec3( 1.0f,  1.0f, -1.0f),
		// Bottom (-Y)
		glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3( 1.0f, -1.0f, -1.0f), glm::vec3( 1.0f, -1.0f,  1.0f),
		glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3( 1.0f, -1.0f,  1.0f), glm::vec3(-1.0f, -1.0f,  1.0f),
	};

	const std::array colors = {
		// Front (red)
		glm::vec3(1, 0, 0), glm::vec3(1, 0, 0), glm::vec3(1, 0, 0),
		glm::vec3(1, 0, 0), glm::vec3(1, 0, 0), glm::vec3(1, 0, 0),
		// Back (green)
		glm::vec3(0, 1, 0), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0),
		glm::vec3(0, 1, 0), glm::vec3(0, 1, 0), glm::vec3(0, 1, 0),
		// Left (blue)
		glm::vec3(0, 0, 1), glm::vec3(0, 0, 1), glm::vec3(0, 0, 1),
		glm::vec3(0, 0, 1), glm::vec3(0, 0, 1), glm::vec3(0, 0, 1),
		// Right (yellow)
		glm::vec3(1, 1, 0), glm::vec3(1, 1, 0), glm::vec3(1, 1, 0),
		glm::vec3(1, 1, 0), glm::vec3(1, 1, 0), glm::vec3(1, 1, 0),
		// Top (magenta)
		glm::vec3(1, 0, 1), glm::vec3(1, 0, 1), glm::vec3(1, 0, 1),
		glm::vec3(1, 0, 1), glm::vec3(1, 0, 1), glm::vec3(1, 0, 1),
		// Bottom (cyan)
		glm::vec3(0, 1, 1), glm::vec3(0, 1, 1), glm::vec3(0, 1, 1),
		glm::vec3(0, 1, 1), glm::vec3(0, 1, 1), glm::vec3(0, 1, 1),
	};

	auto pbuf = MirrorBuffer <array <vec3>> ::from(
		device, positions.size(),
		vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	);

	auto pbuf_value = pbuf.new_value();
	pbuf_value.resize(positions.size());
	for (size_t i = 0; i < positions.size(); ++i)
		pbuf_value[i] = positions[i];
	pbuf.write(pbuf_value);

	auto cbuf = MirrorBuffer <array <vec3>> ::from(
		device, colors.size(),
		vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	);
	
	auto cbuf_value = cbuf.new_value();
	cbuf_value.resize(colors.size());
	for (size_t i = 0; i < colors.size(); ++i)
		cbuf_value[i] = colors[i];
	cbuf.write(cbuf_value);

	// Camera
	Aperature aperature;
	Transform xform;

	xform.translation = glm::vec3(0, 0, 20);
	aperature.aspect = static_cast <float> (window.extent().width) / window.extent().height;

	// TODO: if we have a conditional mirror buffer with prepopulated
	// flags, then move extra flags to the last param...
	auto view_buf = MirrorBuffer <View> ::from(
		device,
		vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	);

	view_buf.write($mirror(View) {
		.proj = aperature.projection_matrix(),
		.view = xform.view_matrix(),
		.model = glm::mat4(1.0f),
	});

	auto pool_size = vk::DescriptorPoolSize()
		.setType(vk::DescriptorType::eUniformBuffer)
		.setDescriptorCount(1);

	auto descriptor_pool = device.logical.createDescriptorPool(
		vk::DescriptorPoolCreateInfo()
			.setMaxSets(1)
			.setPoolSizes(pool_size),
		nullptr,
		dld
	);

	auto descriptor_set = device.logical.allocateDescriptorSets(
		vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(descriptor_pool)
			.setSetLayouts(descriptor_set_layout),
		dld
	).front();

	auto view_buf_info = vk::DescriptorBufferInfo()
		.setBuffer(view_buf.handle)
		.setOffset(0)
		.setRange(sizeof(View));

	auto descriptor_write = vk::WriteDescriptorSet()
		.setDstSet(descriptor_set)
		.setDstBinding(0)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setBufferInfo(view_buf_info);

	device.logical.updateDescriptorSets(descriptor_write, nullptr, dld);

	while (window.alive()) {
		window.poll();

		auto frame = window.next_frame();

		auto extent = frame.extent;
		auto angle = static_cast <float> (glfwGetTime());
		auto model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, angle * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
		aperature.aspect = static_cast <float> (extent.width) / extent.height;

		view_buf.write($mirror(View) {
			.proj = aperature.projection_matrix(),
			.view = xform.view_matrix(),
			// .model = model,
			.model = glm::mat4(1.0f),
		});

		// TODO: window.is_pressed(Key::Q)
		if (glfwGetKey(window.handle, GLFW_KEY_Q) == GLFW_PRESS)
			glfwSetWindowShouldClose(window.handle, true);

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
		cmd.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			pipeline.layout,
			0,
			descriptor_set,
			{}
		);
		cmd.bindVertexBuffers(0, { pbuf.handle, cbuf.handle }, { 0, 0 });
		cmd.draw(static_cast <uint32_t> (positions.size()), 1, 0, 0);
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
