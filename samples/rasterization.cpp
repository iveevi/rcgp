#include <mrd.hpp>

#include <ugp.hpp>

#include "util/aperature.hpp"
#include "util/transform.hpp"

struct View {
	mat4 proj;
	mat4 view;
	mat4 model;

	$reflection(proj, view, model);
};

struct Shared {
	vec3 tint;
	f32 scale;

	$reflection(tint, scale);
};

AttributeStream <vec3, layouts::std430> position;
AttributeStream <vec3> normal;

// TODO: should also have a layout; this needs to be written in the shader though
MonoConstantBuffer <View> view;
MonoConstantBuffer <Shared> shared;

auto vs = $vertex $fn($use(position), $use(normal), $use(view), $use(shared)) -> $returns(Position, vec3)
{
	auto mvp = shared.scale * view.proj * view.view * view.model;
	// auto mvp = view.proj * view.view * view.model;
	auto normal_mat = transpose(inverse(mat3(view.model)));
	auto world_n = normalize(normal_mat * normal);
	$return std::tuple(Position(mvp * vec4(position, 1.0)), world_n);
};

// TODO: push constants
MonoConstantBuffer <vec3> light_direction;

auto fs = $fragment $fn($use(light_direction), $use(shared), vec3 normal) -> $returns(vec4)
{
	// $return vec4(0.5 * normalize(normal) + 0.5, 1.0f);
	auto shade = max(dot(light_direction, normal), 0.0f);
	$return vec4(shared.tint * shade, 1.0f);
};

template <typename X, typename Y>
struct Commands {};

// TODO: descriptor mirrors for each resource group...
// for monoX this degenerates to a single descriptor write
// for aggregates we need a proper mirror...

// AttributeStreams := sequence <reference <Stream>...>
// GroupAllocation := sequence <reference <GRV>...>
template <typename AttributeStreams, typename GroupAllocation, size_t Sets>
struct AnnotatedRasterizationPipeline {
	vk::Pipeline handle;
	vk::PipelineLayout layout;
	std::array <vk::DescriptorSetLayout, Sets> dsls;

	// TODO: using VertexBuffer <ref> = ?
	// TODO: using IndexBuffer = ?

	// TODO: new_descriptor <ref> () method

	// TODO: bind_vertex_buffers(...) -> Commands[X, Y]
};

// TODO: mirror buffer needs minimum buffer usage flags...
// template flags...
// e.g. vertex buffer, uniform buffer, storage buffer, index buffer...

// Vertex buffer associated to a stream resource
template <auto &ref>
struct VertexBuffer {};

template <typename ... Ts>
struct find_implicit_context {
	static constexpr auto contexts = std::array { is_implicit_context_v <Ts>... };
	static constexpr auto idx = first_on(contexts);
	using type = Ts...[idx];
};

template <auto &... refs, size_t ... Is>
auto sequence_to_group_allocation_impl(const sequence <reference <refs>...> &, const std::index_sequence <Is...> &)
{
	return std::make_tuple(group_allocation_record <refs, Is> ()...);
}

template <typename ... Ts>
auto sequence_to_group_allocation(const sequence <Ts...> &)
{
	return sequence_to_group_allocation_impl(
		sequence <typename Ts::type...> ::singleton,
		std::make_index_sequence <sizeof...(Ts)> ()
	);
}

// TODO: flesh out with an impl method with handles things recursively...
// TODO: fallback...
template <auto &ref, ShaderStage ... Ss>
auto reference_to_descriptor_bindings(const Device &device, const stage_wrapper <reference <ref>, Ss...> &)
{
	using base = typename reference <ref> ::raw_base;
	using R = expand_reflection_t <base>;

	// TODO: collect all dslbs and pc ranges recursively
	// TODO: if its a resource group then we need to filter it...
	auto dslbs = std::array {
		vk::DescriptorSetLayoutBinding()
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setStageFlags((stage_to_flag(Ss) | ...))
	};

	auto dsl_info = vk::DescriptorSetLayoutCreateInfo()
		.setBindings(dslbs);

	// TODO: return the dsl and pc ranges separately
	return device.logical.createDescriptorSetLayout(dsl_info);
}

template <typename ... Ts>
auto reference_sequence_to_descriptor_bindings(const Device &device, const sequence <Ts...> &)
{
	// TODO: separate arrays for dsl and pc ranges
	return std::array {
		reference_to_descriptor_bindings(device, Ts())...
	};
}

template <typename ... Subpasses>
struct RasterizationPipelineCombinator {
	const Device &device;
	const Compiler &compiler;
	const RenderPass <Subpasses...> &render_pass;
	// TODO: dynamic extent or fixed extent
	const vk::Extent2D &extent;
	bool depth_test;

	template <typename VRet, typename ... As, typename FRet, typename ... Bs>
	auto operator()(shader_stage <ShaderStage::Vertex, VRet, As...> &vertex,
		 	shader_stage <ShaderStage::Fragment, FRet, Bs...> &fragment) const {
		// TODO: check between vshader output and fshader input
		// TODO: store # of attachments required from # of fshader outputs
		using vertex_icontext = find_implicit_context <As...> ::type;
		using fragment_icontext = find_implicit_context <Bs...> ::type;

		// Collect vertex attribute streams
		auto streams0 = sequence <> ::singleton;
		auto streams = add_stream_from_implicit_context(streams0, vertex_icontext());

		// Generate vertex input bindings and attributes
		auto vbindings = sequence_to_vertex_bindings(streams);
		auto vattributes = sequence_to_vertex_attributes(streams);

		// Collect global resources
		auto gvrs0 = sequence <> ::singleton;
		auto gvrs1 = add_gvr_from_implicit_context <ShaderStage::Vertex> (gvrs0, vertex_icontext());
		auto gvrs = add_gvr_from_implicit_context <ShaderStage::Fragment> (gvrs1, fragment_icontext());

		auto alloc = sequence_to_group_allocation(gvrs);
		auto gamap = new_allocation(alloc);
		vs.apply_group_allocation_map(gamap);
		fs.apply_group_allocation_map(gamap);

		// Compile the shaders
		auto vshader = generators::GLSL(vs).generate();
		fmt::println("vertex shader:\n{}", vshader);

		auto fshader = generators::GLSL(fs).generate();
		fmt::println("fragment shader:\n{}", fshader);
	
		auto vspv = compiler.glsl_to_spirv(vshader, EShLangVertex);
		auto fspv = compiler.glsl_to_spirv(fshader, EShLangFragment);

		auto vmodule = compiler.spirv_to_shader_module(vspv);
		auto fmodule = compiler.spirv_to_shader_module(fspv);

		// Generate the pipeline and descriptor set layouts
		auto dsls = reference_sequence_to_descriptor_bindings(device, gvrs);
		auto layout_info = vk::PipelineLayoutCreateInfo().setSetLayouts(dsls);
		auto layout = device.logical.createPipelineLayout(layout_info);

		// Building the pipeline
		auto shader_stages = std::array {
			vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eVertex)
				.setModule(vmodule)
				.setPName("main"),
			vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eFragment)
				.setModule(fmodule)
				.setPName("main"),
		};

		// TODO: new_pipeline_vertex_input_state
		auto vertex_input = vk::PipelineVertexInputStateCreateInfo()
			.setVertexBindingDescriptions(vbindings)
			.setVertexAttributeDescriptions(vattributes);

		auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology(vk::PrimitiveTopology::eTriangleList)
			.setPrimitiveRestartEnable(false);

		auto viewport = vk::Viewport()
			.setX(0.0f)
			.setY(0.0f)
			.setWidth(float(extent.width))
			.setHeight(float(extent.height))
			.setMinDepth(0.0f)
			.setMaxDepth(1.0f);

		auto scissor = vk::Rect2D()
			.setOffset({ 0, 0 })
			.setExtent(extent);

		auto viewport_state = vk::PipelineViewportStateCreateInfo()
			.setViewports(viewport)
			.setScissors(scissor);

		auto rasterization = vk::PipelineRasterizationStateCreateInfo()
			.setDepthClampEnable(false)
			.setRasterizerDiscardEnable(false)
			.setPolygonMode(vk::PolygonMode::eFill)
			.setCullMode(vk::CullModeFlagBits::eBack)
			.setFrontFace(vk::FrontFace::eCounterClockwise)
			.setDepthBiasEnable(false)
			.setLineWidth(1.0f);

		auto multisampling = vk::PipelineMultisampleStateCreateInfo()
			.setRasterizationSamples(vk::SampleCountFlagBits::e1)
			.setSampleShadingEnable(false)
			.setMinSampleShading(1.0f);

		auto color_blend_attachment = vk::PipelineColorBlendAttachmentState()
			.setBlendEnable(false)
			.setColorWriteMask(
				vk::ColorComponentFlagBits::eR
				| vk::ColorComponentFlagBits::eG
				| vk::ColorComponentFlagBits::eB
				| vk::ColorComponentFlagBits::eA
			);

		auto color_blend = vk::PipelineColorBlendStateCreateInfo()
			.setAttachments(color_blend_attachment);

		auto depth_stencil = vk::PipelineDepthStencilStateCreateInfo()
			.setDepthTestEnable(depth_test)
			.setDepthWriteEnable(depth_test)
			.setDepthCompareOp(vk::CompareOp::eLess)
			.setDepthBoundsTestEnable(false)
			.setStencilTestEnable(false);

		auto pipeline_info = vk::GraphicsPipelineCreateInfo()
			.setLayout(layout)
			.setPInputAssemblyState(&input_assembly)
			.setPVertexInputState(&vertex_input)
			.setPRasterizationState(&rasterization)
			.setPMultisampleState(&multisampling)
			.setPColorBlendState(&color_blend)
			.setPDepthStencilState(&depth_stencil)
			.setPViewportState(&viewport_state)
			.setStages(shader_stages)
			.setRenderPass(render_pass)
			.setSubpass(0);

		auto [result, pipeline] = device.logical.createGraphicsPipeline(nullptr, pipeline_info, nullptr);
		assertion(result == vk::Result::eSuccess, "failed to compile pipeline");

		return AnnotatedRasterizationPipeline <
			decltype(streams),
			decltype(gvrs),
			dsls.size()
		> {
			.handle = pipeline,
			.layout = layout,
			.dsls = dsls,
		};
	}
};

// CTAD for aggregate initialization, deducing subpasses from the render pass.
template <typename ... Subpasses>
RasterizationPipelineCombinator(
	const Device &,
	const Compiler &,
	const RenderPass <Subpasses...> &,
	const vk::Extent2D &,
	bool
) -> RasterizationPipelineCombinator <Subpasses...>;

// TODO: work group is a parameter to shaders that is an intrinsic...
// WorkGroup <8, 8> group... group.block_idx, thread_idx and so on
//
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

	auto window = Window::from(session, device);

	const auto depth_format = vk::Format::eD32Sfloat;

	std::vector <Image> depth_images;
	depth_images.reserve(window.images.size());
	for (size_t i = 0; i < window.images.size(); ++i) {
		depth_images.push_back(Image::from(
			device,
			window.extent(),
			depth_format,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			vk::ImageAspectFlagBits::eDepth,
			vk::MemoryPropertyFlagBits::eDeviceLocal
		));
	}

	auto attachments = Attachments();

	auto common = vk::AttachmentDescription()
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined);

	attachments["color"] = common
		.setFormat(window.format)
		.setStoreOp(vk::AttachmentStoreOp::eStore)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	attachments["depth"] = common
		.setFormat(depth_format)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	auto color = attachments.reference("color", vk::ImageLayout::eColorAttachmentOptimal);
	auto depth = attachments.reference("depth", vk::ImageLayout::eDepthStencilAttachmentOptimal);

	auto render_pass = device.new_render_pass(
		attachments,
		subpass(color_attachments { color },
			depth_attachments { depth },
			color_attachments {},
			input_attachments {})
	);

	std::vector <vk::Framebuffer> framebuffers;
	framebuffers.resize(window.images.size());
	for (size_t i = 0; i < window.images.size(); ++i) {
		auto &image = window.images[i];
		auto &depth = depth_images[i];
		std::array attachments_views { image.view, depth.view };
		framebuffers[i] = device.new_framebuffer(
			render_pass,
			attachments_views,
			image.extent
		);
	}

	auto queue = Queue::from(device);
	auto cpool = CommandPool::from(device, queue);
	auto cmd_buffers = device.new_command_buffers(cpool, window.frames_in_flight);

	auto dpool_info = DescriptorPool::Info {
		.max_sets = 1 << 10,
		.uniform_buffers = 1 << 10,
	};

	auto dpool = DescriptorPool::from(device, dpool_info);

	auto combinator = RasterizationPipelineCombinator {
		.device = device,
		.compiler = Compiler::from(device, Compiler::Info()),
		.render_pass = render_pass,
		.extent = window.extent(),
		.depth_test = true,
	};

	auto pipeline = combinator(vs, fs);

	constexpr mrd::ModelEncodings encodings;

	using Model = mrd::Model <encodings>;

	auto mb = [](uint32_t bytes) { return bytes / (1 << 20); };

	auto model = Model::load("/home/venki/data/models/asian-dragon.stl");
	auto &mesh = model.meshes[0];
	fmt::println("# of verts: {}, # of triangles: {}", mesh.positions.size(), mesh.primitives.size());
	fmt::println("bytes: {} MB", mb(mesh.size_bytes()));
	mesh.deduplicate();
	fmt::println("bytes: {} MB", mb(mesh.size_bytes()));

	auto pbuf = VertexMirrorBuffer <array <vec3>, layouts::std430> ::from(
		device, mesh.positions.size(),
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write(span(mesh.positions).map(
		[](auto p) -> $mirror(vec3, layouts::std430) {
			return { p.x, p.y, p.z };
		}
	));

	auto nbuf = VertexMirrorBuffer <array <vec3>, layouts::scalar> ::from(
		device, mesh.normals.size(),
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent,
		vk::BufferUsageFlagBits::eTransferDst
	).write(span(mesh.normals).map(
		[](auto p) -> $mirror(vec3, layouts::scalar) {
			return { p.x, p.y, p.z };
		}
	));

	auto ibuf = IndexMirrorBuffer <array <ivec3>, layouts::scalar> ::from(
		device, mesh.primitives.size(),
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write(span(mesh.primitives).map(
		[](auto p) -> $mirror(ivec3, layouts::scalar) {
			return { p.x, p.y, p.z };
		}
	));

	// Camera
	Aperature aperature;
	Transform xform;

	xform.translation = glm::vec3(0, 0, -10);
	aperature.aspect = float(window.extent().width) / window.extent().height;

	auto view_buf = UniformMirrorBuffer <View, layouts::std430> ::from(
		device,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write($mirror(View) {
		.proj = aperature.projection_matrix(),
		.view = xform.view_matrix(),
		.model = glm::mat4(1.0f),
	});
	
	auto light_buf = UniformMirrorBuffer <vec3, layouts::std430> ::from(
		device,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write(glm::normalize(glm::vec3(1, 1, 1)));
	
	auto shared_buf = UniformMirrorBuffer <Shared, layouts::std430> ::from(
		device,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write($mirror(Shared) {
		.tint = glm::vec3(1, 0.5, 0.5),
		.scale = 1.5f,
	});

	auto dsets = device.new_descriptor_sets(dpool, pipeline.dsls);

	auto view_buf_info = view_buf.descriptor_info();
	auto view_write = vk::WriteDescriptorSet()
		.setDstSet(dsets[0])
		.setDstBinding(0)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setBufferInfo(view_buf_info);
	
	auto shared_buf_info = shared_buf.descriptor_info();
	auto shared_write = vk::WriteDescriptorSet()
		.setDstSet(dsets[1])
		.setDstBinding(0)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setBufferInfo(shared_buf_info);
	
	auto light_buf_info = light_buf.descriptor_info();
	auto light_write = vk::WriteDescriptorSet()
		.setDstSet(dsets[2])
		.setDstBinding(0)
		.setDescriptorType(vk::DescriptorType::eUniformBuffer)
		.setBufferInfo(light_buf_info);

	device.logical.updateDescriptorSets(
		{ view_write, shared_write, light_write },
		nullptr, dld
	);

	window.on_drag(GLFW_MOUSE_BUTTON_LEFT, [&](double, double, double dx, double dy) {
		// Rotate camera in place (viewport tumble): yaw around world up, pitch around camera right.
		const float rot_speed = 0.005f; // radians per pixel
		auto yaw = glm::angleAxis(-float(dx * rot_speed), glm::vec3(0.0f, 1.0f, 0.0f));
		auto pitch = glm::angleAxis(float(dy * rot_speed), xform.right());
		xform.rotation = glm::normalize(pitch * yaw * xform.rotation);
	});

	glfwSetInputMode(window.handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	while (window.alive()) {
		window.poll();

		static double last_time = glfwGetTime();
		double now = glfwGetTime();
		float dt = float(now - last_time);
		last_time = now;

		const float move_speed = 25.0f;

		glm::vec3 move(0.0f);
		if (window.is_pressed(Key::W)) move.z += 1.0f;
		if (window.is_pressed(Key::S)) move.z -= 1.0f;
		if (window.is_pressed(Key::A)) move.x += 1.0f;
		if (window.is_pressed(Key::D)) move.x -= 1.0f;
		if (window.is_pressed(Key::E)) move.y += 1.0f;
		if (window.is_pressed(Key::Q)) move.y -= 1.0f;
		if (glm::length(move) > 0.0f) {
			auto local = move.z * xform.forward()
				+ move.x * xform.right()
				+ move.y * xform.up();
			xform.translation += glm::normalize(local) * move_speed * dt;
		}

		auto frame = window.next_frame();

		auto extent = frame.extent;
		auto angle = static_cast <float> (glfwGetTime());
		auto model = glm::mat4(1.0f);
		model = glm::scale(model, glm::vec3(0.1));
		// auto model = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
		// model = glm::rotate(model, angle * 0.5f, glm::vec3(1.0f, 0.0f, 0.0f));
	
		aperature.aspect = static_cast <float> (extent.width) / extent.height;

		view_buf.write($mirror(View) {
			.proj = aperature.projection_matrix(),
			.view = xform.view_matrix(),
			.model = model,
		});

		light_buf.write(glm::normalize(glm::vec3(sin(angle), cos(2 * angle), sin(2 - angle))));

		if (window.is_pressed(Key::Escape))
			window.close();

		device.wait_for_frame(frame);
		if (!device.acquire_image_for_frame(frame))
			continue;

		auto &cmd = cmd_buffers[window.frame_index];

		cmd.reset();
		cmd.begin(vk::CommandBufferBeginInfo());

		auto render_area = vk::Rect2D()
			.setOffset({ 0, 0 })
			.setExtent(frame.extent);

		auto clear_values = std::array {
			vk::ClearValue().setColor(vk::ClearColorValue(std::array <float, 4> { 0.05f, 0.05f, 0.05f, 1.0f })),
			vk::ClearValue().setDepthStencil(vk::ClearDepthStencilValue(1.0f, 0)),
		};

		auto rp_begin = vk::RenderPassBeginInfo()
			.setRenderPass(render_pass)
			.setFramebuffer(framebuffers[frame.image_index])
			.setRenderArea(render_area)
			.setClearValues(clear_values);

		cmd.beginRenderPass(rp_begin, vk::SubpassContents::eInline);
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.handle);
		cmd.bindDescriptorSets(
			vk::PipelineBindPoint::eGraphics,
			pipeline.layout,
			0, dsets, {}
		);
		cmd.bindVertexBuffers(0, { pbuf.handle, nbuf.handle }, { 0, 0 });
		cmd.bindIndexBuffer(ibuf.handle, 0, vk::IndexType::eUint32);
		cmd.drawIndexed(3 * mesh.primitives.size(), 1, 0, 0, 0);
		cmd.endRenderPass();
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
