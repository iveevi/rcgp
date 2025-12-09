#include <array>
#include <vector>
#include <cstdint>

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

AttributeStream <vec3> position;
AttributeStream <vec3> normal;

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

// TODO: push constant
MonoConstantBuffer <vec3> light_direction;

auto fs = $fragment $fn($use(light_direction), $use(shared), vec3 normal) -> $returns(vec4)
{
	// $return vec4(0.5 * normalize(normal) + 0.5, 1.0f);
	auto shade = max(dot(light_direction, normal), 0.0f);
	$return vec4(shared.tint * shade, 1.0f);
};

template <typename T>
struct command_state {};

// struct TGP {
// 	auto dispatch() {
// 		return command_state <int> ();
// 	}
// };

template <typename ... Ts>
struct AnnotatedRasterizationPipeline {
	vk::Pipeline handle;
	vk::PipelineLayout layout;
	std::vector <vk::DescriptorSetLayout> dsls;

	using VertexBuffer = void;
	// using IndexBuffer = void; TODO: ?
};

auto begin_command_buffer()
{
	return command_state <int> ();
}

template <typename T, Stage ... Ss>
struct stage_wrapper {
	using stages = std::integer_sequence <Stage, Ss...>;
	using type = T;

	template <Stage S>
	using append_stage = std::conditional_t <
		((Ss == S) || ...),
		stage_wrapper <T, Ss...>,
		stage_wrapper <T, Ss..., S>
	>;
};

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

template <Stage S, auto &ref, typename ... Ts>
auto add_global_resource(const sequence <Ts...> &in)
{
	using base = reference <ref> ::raw_base;
	if constexpr (!is_global_resource_v <base>) {
		return in;
	} else {
		constexpr auto exists = (std::same_as <typename Ts::type, reference <ref>> || ...);

		if constexpr (exists) {
			return sequence <
				std::conditional_t <
					std::same_as <typename Ts::type, reference <ref>>,
					typename Ts::template append_stage <S>,
					Ts
				> ...
			> ::singleton;
		} else {
			return sequence <Ts..., stage_wrapper <reference <ref>, S>> ::singleton;
		}
	}
}

template <Stage S, typename ... Ts, auto &b, auto &...bs>
auto add_implicit_context(const sequence <Ts...> &in, const implicit_context <b, bs...> &)
{
	auto out = add_global_resource <S, b> (in);
	if constexpr (sizeof...(bs))
		return add_implicit_context <S> (out, implicit_context <bs...> ());
	else
		return out;
}

template <typename ... Ts>
struct RasterizationPipelineCombinator {
	// TODO: options...

	template <typename VRet, typename ... As, typename FRet, typename ... Bs>
	auto operator()(stage <Stage::Vertex, VRet, As...> &vertex,
		 	stage <Stage::Fragment, FRet, Bs...> &fragment)
	{
		// TODO: check between vshader output and fshader input
		// TODO: store # of attachments required from # of fshader outputs
		using vertex_icontext = find_implicit_context <As...> ::type;
		using fragment_icontext = find_implicit_context <Bs...> ::type;

		auto grvs0 = sequence <> ::singleton;
		auto grvs1 = add_implicit_context <Stage::Vertex> (grvs0, vertex_icontext());
		auto grvs = add_implicit_context <Stage::Fragment> (grvs1, fragment_icontext());

		auto alloc = sequence_to_group_allocation(grvs);
		auto gamap = new_allocation(alloc);
		vs.apply_group_allocation_map(gamap);
		fs.apply_group_allocation_map(gamap);

		return grvs;
	}
};

template <typename ... Ts>
using rpc = RasterizationPipelineCombinator <Ts...>;

// TODO: structs need a layout (defaults to std430)
// 
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

	auto queue = Queue::from(device);
	auto cpool = CommandPool::from(device, queue);
	auto cmdbuffers = group(device, cpool).allocate(window.frames_in_flight);

	auto dpool_info = DescriptorPool::Info {
		.max_sets = 1 << 10,
		.uniform_buffers = 1 << 10,
	};

	auto dpool = DescriptorPool::from(device, dpool_info);

	auto compiler = Compiler::from(device, Compiler::Info());

	// TODO: comb from compiler and other options...
	auto comb = rpc();
	auto c = comb(vs, fs);

	// auto alloc = sequence_to_group_allocation(c);
	// auto map = new_allocation(alloc);
	// vs.apply_group_allocation_map(map);
	// fs.apply_group_allocation_map(map);

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
			.setStride(sizeof(glm::vec3))
			.setInputRate(vk::VertexInputRate::eVertex),
		vk::VertexInputBindingDescription()
			.setBinding(1)
			.setStride(sizeof(glm::vec3))
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

	// TODO: should be able to construct the pipeline layout from the group allocation
	auto view_binding = vk::DescriptorSetLayoutBinding()
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eVertex);

	auto light_binding = vk::DescriptorSetLayoutBinding()
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eFragment);

	auto shared_binding = vk::DescriptorSetLayoutBinding()
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(vk::DescriptorType::eUniformBuffer)
			.setStageFlags(vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);

	auto view_dsl = device.logical.createDescriptorSetLayout(
		vk::DescriptorSetLayoutCreateInfo().setBindings(view_binding),
		nullptr,
		dld
	);
	
	auto light_dsl = device.logical.createDescriptorSetLayout(
		vk::DescriptorSetLayoutCreateInfo().setBindings(light_binding),
		nullptr,
		dld
	);
	
	auto shared_dsl = device.logical.createDescriptorSetLayout(
		vk::DescriptorSetLayoutCreateInfo().setBindings(shared_binding),
		nullptr,
		dld
	);

	// auto dsls = std::array { view_dsl, light_dsl, shared_dsl };
	auto dsls = std::array { view_dsl, shared_dsl, light_dsl };

	auto pipeline_layout = device.logical.createPipelineLayout(
		vk::PipelineLayoutCreateInfo().setSetLayouts(dsls),
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
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

	attachments["depth"] = vk::AttachmentDescription()
		.setFormat(depth_format)
		.setSamples(vk::SampleCountFlagBits::e1)
		.setLoadOp(vk::AttachmentLoadOp::eClear)
		.setStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
		.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
		.setInitialLayout(vk::ImageLayout::eUndefined)
		.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

	auto color = attachments.reference("color", vk::ImageLayout::eColorAttachmentOptimal);
	auto depth = attachments.reference("depth", vk::ImageLayout::eDepthStencilAttachmentOptimal);

	auto rp = group(device, dld).new_renderpass(
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
		framebuffers[i] = group(device, rp, dld).new_framebuffer(
			attachments_views,
			image.extent,
			1
		);
	}

	// TODO: shoudl eventually remove RasterizationPipeline
	auto pipeline_info = RasterizationPipeline::Info {
		.vmodule = vmodule,
		.fmodule = fmodule,
		.renderpass = rp,
		.extent = window.extent(),
		.bindings = binding_descs,
		.attributes = attribute_descs,
		.layout = pipeline_layout,
		.depth_test = true,
	};

	auto pipeline = RasterizationPipeline::from(device, dld, pipeline_info);

	constexpr mrd::ModelEncodings encodings;

	using Model = mrd::Model <encodings>;

	auto mb = [](uint32_t bytes) { return bytes / (1 << 20); };

	auto model = Model::load("/home/venki/data/models/asian-dragon.stl");
	auto &mesh = model.meshes[0];
	fmt::println("# of verts: {}, # of triangles: {}", mesh.positions.size(), mesh.primitives.size());
	fmt::println("bytes: {} MB", mb(mesh.size_bytes()));
	mesh.deduplicate();
	fmt::println("bytes: {} MB", mb(mesh.size_bytes()));

	auto pbuf = MirrorBuffer <array <vec3>> ::from(
		device, mesh.positions.size(),
		vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	);

	auto pbuf_value = pbuf.new_value();
	pbuf_value.resize(mesh.positions.size());
	std::memcpy(pbuf_value.data(), mesh.positions.data(), pbuf_value.size_bytes());
	pbuf.write(pbuf_value);

	auto nbuf = MirrorBuffer <array <vec3>> ::from(
		device, mesh.normals.size(),
		vk::BufferUsageFlagBits::eVertexBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible
			| vk::MemoryPropertyFlagBits::eHostCoherent
	);

	auto nbuf_value = nbuf.new_value();
	nbuf_value.resize(mesh.normals.size());
	std::memcpy(nbuf_value.data(), mesh.normals.data(), nbuf_value.size_bytes());
	nbuf.write(nbuf_value);

	auto ibuf = MirrorBuffer <array <ivec3>, layouts::scalar> ::from(
		device, mesh.primitives.size(),
		vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible
			| vk::MemoryPropertyFlagBits::eHostCoherent
	);

	auto ibuf_value = ibuf.new_value();
	ibuf_value.resize(mesh.primitives.size());
	std::memcpy(ibuf_value.data(), mesh.primitives.data(), sizeof(glm::ivec3) * ibuf_value.size());
	ibuf.write(ibuf_value);

	// Camera
	Aperature aperature;
	Transform xform;

	xform.translation = glm::vec3(0, 0, -10);
	aperature.aspect = static_cast <float> (window.extent().width) / window.extent().height;

	// TODO: if we have a conditional mirror buffer with prepopulated
	// flags, then move extra flags to the last param...
	auto view_buf = MirrorBuffer <View> ::from(
		device,
		vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write($mirror(View) {
		.proj = aperature.projection_matrix(),
		.view = xform.view_matrix(),
		.model = glm::mat4(1.0f),
	});
	
	auto light_buf = MirrorBuffer <vec3> ::from(
		device,
		vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write(glm::normalize(glm::vec3(1, 1, 1)));
	
	auto shared_buf = MirrorBuffer <Shared> ::from(
		device,
		vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write($mirror(Shared) {
		.tint = glm::vec3(1, 0.5, 0.5),
		.scale = 1.5f,
	});

	auto dsets = device.logical.allocateDescriptorSets(
		vk::DescriptorSetAllocateInfo()
			.setDescriptorPool(dpool)
			.setSetLayouts(dsls),
		dld
	);

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

	// TODO: during command seq we should auto batch descriptor set
	// operations and do one updatedescriptorsets call just before dispatch
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

		group(device, window).wait(frame);
		auto acquired = group(device, window).acquire_image(frame);
		if (!acquired)
			continue;

		auto &cmd = cmdbuffers[window.frame_index];

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
			.setRenderPass(rp)
			.setFramebuffer(framebuffers[frame.image_index])
			.setRenderArea(render_area)
			.setClearValues(clear_values);

		cmd.beginRenderPass(rp_begin, vk::SubpassContents::eInline);
		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);
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
