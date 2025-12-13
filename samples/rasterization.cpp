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

// TODO: ConstantBuffer -> UniformBuffer
// TODO: ResourceGroups of aggregates should
// purely consisnt of resource handles...
// explicit push constants via PushConstants <T>
// with sequenital non overlapping pc ranges

// TODO: should also have a layout; this needs to be written in the shader though
MonoConstantBuffer <View> view;
// TODO: we should allow standalone resource handles to act as references...
// then we can avoid the hokey pokey of referece_base being different/specialized
// in any case ray payloads would be standalone references
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
MonoConstantBuffer <vec3> light;

auto fs = $fragment $fn($use(light), $use(shared), vec3 normal) -> $returns(vec4)
{
	// $return vec4(0.5 * normalize(normal) + 0.5, 1.0f);
	auto shade = max(dot(light, normal), 0.0f);
	$return vec4(shared.tint * shade, 1.0f);
};

// Primary command buffer state
// ----------------------------
//  * Any
//  * Neutral
//  * RenderPass
//  * RasterizationPipeline
enum class CommandFlag {
	aAny,
	eNeutral,
	eRenderPass,
	eRasterizationPipeline,
};
// TODO: decay rules:
// (Rasterization/MeshShading)Pipeline -> RenderPass
// (RayTracing/Compute)Pipeline -> Neutral
// RenderPass -> Neutral

// Auxiliary command buffer state as dependency information
// TODO: flags for phase?
template <auto &ref>
struct PendingAttributeStream {};

template <auto &ref>
struct PendingGlobalResource {};

// TODO: sync stuff goes here
// NOTE: in general there are various aux infos
// but they have an algebra

// NOTE: RPending and RProvided cancel each other out
// if they ref the same thing...

// TODO: concept for auxiliary states

// Ts are all auxiliary states
template <CommandFlag S, typename ... Ts>
struct CommandState {
	static_assert(!((S == CommandFlag::eNeutral) && (sizeof...(Ts) > 0)),
		 "neutral states should have no other state");

	// TODO: append aux <T> -> <Ts..., T>
};

using NeutralCommandState = CommandState <CommandFlag::eNeutral>;

// TODO: concept for command state

struct CommandsTraceAux {
	vk::PipelineBindPoint bind_point;
	vk::PipelineLayout layout;
	// TODO: also track pipeline stage for syncing
};

using command_operator = std::function <void (const vk::CommandBuffer &, CommandsTraceAux &)>;

// TODO: profile/benchmark performance of commands
// X := CommandState <X, ...>
// Y := CommandState <Y, ...>
template <typename X, typename Y>
struct Commands : std::vector <command_operator> {
	using std::vector <command_operator> ::vector;

	auto &operator()(const vk::CommandBuffer &cmd) const {
		static_assert(std::same_as <X, NeutralCommandState>
			&& std::same_as <Y, NeutralCommandState>);

		CommandsTraceAux aux;

		cmd.reset();
		cmd.begin(vk::CommandBufferBeginInfo());

		for (auto &op : *this)
			op(cmd, aux);

		cmd.end();

		return cmd;
	}
};

using NeutralCommands = Commands <NeutralCommandState, NeutralCommandState>;

template <typename A, typename B, typename C, typename D>
auto seq(const Commands <A, B> &x, const Commands <C, D> &y)
{
	// TODO: general fusion operator <A, B, C, D> -> Commands <F::in, F::out>
	// TODO: is_compatible(B::flag, C::flag)
	// TODO: static assertions... fallback to agnostic if all else fails
	auto cmd = Commands <A, D> {};
	cmd.append_range(x);
	cmd.append_range(y);
	return cmd;
}

template <typename First, typename Second, typename ...Rest>
auto seq(const First &first, const Second &second, const Rest &... rest)
{
	static_assert(sizeof...(Rest) >= 0, "seq requires at least two arguments");
	auto combined = seq(first, second);
	if constexpr (sizeof...(Rest) == 0)
		return combined;
	else
		return seq(combined, rest...);
}

// Pipe operator: syntactic sugar for seq
template <typename A, typename B, typename C, typename D>
auto operator|(const Commands <A, B> &lhs, const Commands <C, D> &rhs)
{
	return seq(lhs, rhs);
}

auto begin_render_pass(const vk::RenderPass &render_pass,
		       const vk::Framebuffer &framebuffer,
		       const vk::Rect2D &render_area,
		       const std::span <vk::ClearValue> &clear_values)
{
	auto binder = [=](const vk::CommandBuffer &cmd, CommandsTraceAux &) {
		auto rp_begin = vk::RenderPassBeginInfo()
			.setRenderPass(render_pass)
			.setFramebuffer(framebuffer)
			.setRenderArea(render_area)
			.setClearValues(clear_values);

		cmd.beginRenderPass(rp_begin, vk::SubpassContents::eInline);
	};

	return Commands <
		NeutralCommandState,
		CommandState <CommandFlag::eRenderPass>
	> { binder };
}

template <typename AttributeStreams, typename GlobalResources, size_t Sets>
auto bind_pipeline(const AnnotatedRasterizationPipeline <AttributeStreams, GlobalResources, Sets> &pipeline)
{
	// TODO: write bind point and layout in an interm state (CommandBufferAux &)?
	auto binder = [=](const vk::CommandBuffer &cmd, CommandsTraceAux &aux) {
		aux.bind_point = vk::PipelineBindPoint::eGraphics;
		aux.layout = pipeline.layout;
		cmd.bindPipeline(aux.bind_point, pipeline.handle);
	};

	return Commands <
		CommandState <CommandFlag::eRenderPass>,
		// TODO: put resource dependencies for all
		// streams and global resources
		CommandState <CommandFlag::eRasterizationPipeline>
	> { binder };
}

// TODO: should have some additional metadata for each descriptor
// which unambiguously identifies the pipeline layout... since
// descriptor set and stage usage may differ
//
// TODO: since this ordering is resolved statically, its probably sufficient
// to generate a unique type index for the pipeline, and then transfer
// that as an additional label to the descriptors;
// then everything can be resolved with the pendingX by attaching
// the ids to the pending states...
// use friend injection to register the pipeline's type under that generated index
//
// TODO: generally this type id + friend injection techique could be used
// to obfuscate/hide the actual type information from users (and compiler)
// to relieve some of the burden of looking at long types...
template <auto &... refs>
auto bind_descriptors(const DescriptorOf <refs, true> &... descriptors)
{
	// TODO: take resolved as template parameters, static assert if not all resolved

	// TODO: pass offset of first set as well
	auto binder = [=](const vk::CommandBuffer &cmd, CommandsTraceAux &aux) {
		// TODO: find the set index of each from the pipeline
		// (by looking its type up from the index)

		// NOTE: for now assuming its in order and all at once
		
   		size_t idx = 0;
		(cmd.bindDescriptorSets(
			aux.bind_point,
			aux.layout,
			idx++, { descriptors }, {}
		), ...);
	};

	// TODO: fetch the pipeline type associated to the descriptor pipeline id
	return Commands <
		CommandState <CommandFlag::eRasterizationPipeline>,
		CommandState <CommandFlag::eRasterizationPipeline>
	> { binder };
}

auto unbind_pipeline()
{
	// Vulkan has no explicit unbind; this is a logical state transition helper.
	auto binder = [](const vk::CommandBuffer &, CommandsTraceAux &) {};

	return Commands <
		CommandState <CommandFlag::eRasterizationPipeline>,
		CommandState <CommandFlag::eRenderPass>
	> { binder };
}

template <auto &... refs>
auto bind_vertex_buffers(const VertexBufferOf <refs> &... buffers)
{
	auto binder = [=](const vk::CommandBuffer &cmd, CommandsTraceAux &) {
		std::array <vk::DeviceSize, sizeof...(refs)> offsets {};
		cmd.bindVertexBuffers(0, { buffers.handle... }, offsets);
	};

	return Commands <
		CommandState <CommandFlag::eRasterizationPipeline>,
		CommandState <CommandFlag::eRasterizationPipeline>
	> { binder };
}

auto bind_index_buffer(const Buffer &buffer)
{
	// TODO: templates for inferring the index type
	auto binder = [=](const vk::CommandBuffer &cmd, CommandsTraceAux &) {
		cmd.bindIndexBuffer(buffer.handle, 0, vk::IndexType::eUint32);
	};

	return Commands <
		CommandState <CommandFlag::eRasterizationPipeline>,
		CommandState <CommandFlag::eRasterizationPipeline>
	> { binder };
}

auto draw_indexed(uint32_t count)
{
	// TODO: should return an aux state which adds back the dependencies
	auto binder = [=](const vk::CommandBuffer &cmd, CommandsTraceAux &) {
		cmd.drawIndexed(count, 1, 0, 0, 0);
	};

	return Commands <
		CommandState <CommandFlag::eRasterizationPipeline>,
		CommandState <CommandFlag::eRasterizationPipeline>
	> { binder };
}

auto end_render_pass()
{
	auto binder = [](const vk::CommandBuffer &cmd, CommandsTraceAux &) {
		cmd.endRenderPass();
	};

	return Commands <
		CommandState <CommandFlag::eRenderPass>,
		NeutralCommandState
	> { binder };
}

// TODO: flow should be:
// commands = seq(...)
// queue.submit(commands(cmd));
// can make it secondary via
// device.make_secondary_commands(commands1, ...)

// TODO: IndexBuffer <topology, index type>, then is translated
// accordingly when fed to pipeline draw commands; but
// topology MUST match; also need to match topology to
// a canonical representation

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
		// TODO: image info struct...
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

	auto pbuf = VertexBufferOf <position> ::from(
		device, mesh.positions.size(),
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write(span(mesh.positions).map(
		[](auto p) -> DynamicElementOf <position> {
			return { p.x, p.y, p.z };
		}
	));

	auto nbuf = VertexBufferOf <normal> ::from(
		device, mesh.normals.size(),
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent,
		vk::BufferUsageFlagBits::eTransferDst
	).write(span(mesh.normals).map(
		[](auto p) -> DynamicElementOf <normal> {
			return { p.x, p.y, p.z };
		}
	));

	auto ibuf = IndexMirrorBuffer <array <ivec3>, layouts::scalar> ::from(
		device, mesh.primitives.size(),
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write(span(mesh.primitives).map(
		// TODO: -> DynamicElementOf <?> tis unclear...
		[](auto p) -> TypeMirror <ivec3, layouts::scalar> {
			return { p.x, p.y, p.z };
		}
	));

	// Camera
	Aperature aperature;
	Transform xform;

	xform.translation = glm::vec3(0, 0, -10);
	aperature.aspect = float(window.extent().width) / window.extent().height;

	// TODO: XOf or similar alias for command buffers?
	auto view_buf = ResourceOf <view> ::from(
		device,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write(MirrorOf <view> {
		.proj = aperature.projection_matrix(),
		.view = xform.view_matrix(),
		.model = glm::mat4(1.0f),
	});
	
	auto light_buf = ResourceOf <light> ::from(
		device,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write(glm::normalize(glm::vec3(1, 1, 1)));
	
	auto shared_buf = ResourceOf <shared> ::from(
		device,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write(MirrorOf <shared> {
		.tint = glm::vec3(1, 0.5, 0.5),
		.scale = 1.5f,
	});
	
	auto view_descriptor = pipeline.new_descriptor <view> (dpool);
	auto shared_descriptor = pipeline.new_descriptor <shared> (dpool);
	auto light_descriptor = pipeline.new_descriptor <light> (dpool);

	auto [view_ds, shared_ds, light_ds] = device.update_descriptors(
		DescriptorWritePair { view_descriptor, view_buf },
		DescriptorWritePair { shared_descriptor, shared_buf },
		DescriptorWritePair { light_descriptor, light_buf }
		// TODO: DescriptorCopyPair or something?
	);

	// TODO: enum for mouse buttons
	window.on_drag(GLFW_MOUSE_BUTTON_LEFT, [&](double, double, double dx, double dy) {
		const float speed = 0.005f;
		auto yaw = glm::angleAxis(-float(dx * speed), glm::vec3(0.0f, 1.0f, 0.0f));
		auto pitch = glm::angleAxis(float(dy * speed), xform.right());
		xform.rotation = glm::normalize(pitch * yaw * xform.rotation);
	});

	// TODO: window method for this...
	glfwSetInputMode(window.handle, GLFW_RAW_MOUSE_MOTION, true);

	auto commands = [&](const Frame &frame) {
		auto render_area = vk::Rect2D()
			.setOffset({ 0, 0 })
			.setExtent(frame.extent);

		auto clear_values = std::array <vk::ClearValue, 2> {
			vk::ClearColorValue().setFloat32({ 0.05f, 0.05f, 0.05f, 1.0f }),
			vk::ClearDepthStencilValue(1.0f, 0),
		};

		auto a = begin_render_pass(render_pass, framebuffers[frame.image_index], render_area, clear_values);
		auto b = bind_pipeline(pipeline);
		auto c = bind_descriptors(view_ds, shared_ds, light_ds);
		auto d = bind_vertex_buffers(pbuf, nbuf);
		auto e = bind_index_buffer(ibuf);
		auto f = draw_indexed(3 * mesh.primitives.size());
		auto g = unbind_pipeline();
		auto h = end_render_pass();

		return a | b | c | d | e | f | g | h;
	};

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

		view_buf.write(TypeMirror <View> {
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

		// auto &cmd = commands(frame)(cmd_buffers[window.frame_index]);
		
		auto render_area = vk::Rect2D()
			.setOffset({ 0, 0 })
			.setExtent(frame.extent);

		auto clear_values = std::array <vk::ClearValue, 2> {
			vk::ClearColorValue().setFloat32({ 0.05f, 0.05f, 0.05f, 1.0f }),
			vk::ClearDepthStencilValue(1.0f, 0),
		};

		auto a = begin_render_pass(render_pass, framebuffers[frame.image_index], render_area, clear_values);
		auto b = bind_pipeline(pipeline);
		auto c = bind_descriptors(view_ds, shared_ds, light_ds);
		auto d = bind_vertex_buffers(pbuf, nbuf);
		auto e = bind_index_buffer(ibuf);
		auto f = draw_indexed(3 * mesh.primitives.size());
		auto g = unbind_pipeline();
		auto h = end_render_pass();

		auto commands = seq(a, b, c, d, e, f, g, h);

		// TODO: would be nice to allocate actual handles to neutral cmds...
		auto &cmd = cmd_buffers[window.frame_index];

		queue.submit(commands(cmd),
	        	frame.presented,
	        	frame.rendered,
	        	vk::PipelineStageFlagBits::eColorAttachmentOutput,
	        	frame.fence
	        );

		auto result = queue.present(
			window.swapchain,
			frame.image_index,
			frame.rendered
		);
	}
}
