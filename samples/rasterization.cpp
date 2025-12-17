#include <mrd.hpp>

#include <ugp.hpp>

#include "util/aperature.hpp"
#include "util/transform.hpp"

constexpr mrd::MeshEncodings mesh_encodings;
constexpr mrd::ModelEncodings model_encodings {
	.meshes = mesh_encodings,
};

using Model = mrd::Model <model_encodings>;
using Mesh = mrd::Mesh <mesh_encodings>;

struct View {
	mat4 proj;
	mat4 view;
	mat4 model;

	$reflection(proj, view, model);
};

AttributeStream <vec3> position;
AttributeStream <vec3> normal;

UniformBuffer <View> view;

auto vs = $vertex $fn($ref(position), $ref(normal), $ref(view)) -> $returns(Position, Smooth <vec3>)
{
	auto mvp = view.proj * view.view * view.model;
	auto nmat = transpose(inverse(mat3(view.model)));
	auto wn = normalize(mat3(view.model) * normal);
	auto pp = mvp * vec4(position, 1.0);
	$return std::tuple(pp, wn);
};

// TODO: push constants
UniformBuffer <vec3> light;

auto fs = $fragment $fn($ref(light), vec3 world_normal) -> $returns(vec4)
{
	auto normal = normalize(world_normal);
	auto shade = max(dot(light, normal), 0.0f);
	$return vec4(vec3(1, 0.5, 0.5) * shade, 1.0f);
};

using TriangleBuffer = IndexBuffer <Topology::eTriangleList, uint32_t>;

struct MeshHandle {
	size_t draw_count;
	VertexBufferOf <position> positions;
	VertexBufferOf <normal> normals;
	TriangleBuffer triangles;

	static auto from(const Device &device, const Mesh &mesh) {
		MeshHandle result;

		result.draw_count = 3 * mesh.primitives.size();

		result.positions = VertexBufferOf <position> ::from(
			device, mesh.positions.size(),
			vk::MemoryPropertyFlagBits::eHostVisible
			| vk::MemoryPropertyFlagBits::eHostCoherent
		).write(span(mesh.positions).map(
			[](auto p) -> DynamicDataTypeOf <position> {
				return { p.x, p.y, p.z };
			}
		));
	
		result.normals = VertexBufferOf <normal> ::from(
			device, mesh.normals.size(),
			vk::MemoryPropertyFlagBits::eHostVisible
			| vk::MemoryPropertyFlagBits::eHostCoherent
		).write(span(mesh.normals).map(
			[](auto p) -> DynamicDataTypeOf <normal> {
				return { p.x, p.y, p.z };
			}
		));

		result.triangles = TriangleBuffer::from(
			device, mesh.primitives.size(),
			vk::MemoryPropertyFlagBits::eHostVisible
			| vk::MemoryPropertyFlagBits::eHostCoherent
		).write(span(mesh.primitives).map(
			[](auto p) -> DynamicDataType <TriangleBuffer> {
				return { p.x, p.y, p.z };
			}
		));

		return result;
	}
};

auto draw_mesh(const MeshHandle &handles)
{
	return nullptr
		| bind_vertex_buffers(handles.positions, handles.normals)
		| bind_index_buffer(handles.triangles)
		| draw_indexed(handles.draw_count);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		error("expected model file");
		return EXIT_FAILURE;
	}

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
		auto image_info = Image::Info {
			.extent = window.extent(),
			.format = depth_format,
			.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
			.properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
			.aspect = vk::ImageAspectFlagBits::eDepth,
		};

		depth_images.push_back(Image::from(device, image_info));
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
			image.info.extent
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

	auto compiler = Compiler::from(device, Compiler::Info());
	
	auto combinator = RasterizationCombinator {
		.topology = marker_v <Topology::eTriangleList>,
		.device = device,
		.compiler = compiler,
		.render_pass = render_pass,
		.options = RasterizationOptions {
			.extent = window.extent(),
			.depth_test = true
		}
	};

	auto pipeline = combinator(vs, fs);

	auto model = Model::load(argv[1]);
	auto &mesh = model.meshes[0];
	mesh.recalculate_normals();

	auto handle = MeshHandle::from(device, mesh);

	// Camera
	Aperature aperature;
	Transform xform;

	xform.translation = glm::vec3(0, 0, -10);
	aperature.aspect = float(window.extent().width) / window.extent().height;

	auto view_buf = ResourceMirrorOf <view> ::from(
		device,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write(DataTypeOf <view> {
		.proj = aperature.projection_matrix(),
		.view = xform.view_matrix(),
		.model = glm::mat4(1.0f),
	});
	
	auto light_buf = ResourceMirrorOf <light> ::from(
		device,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write(glm::vec3(0, 1, 0));
	
	auto raw_view_ds = pipeline.new_descriptor <view> (dpool);
	auto raw_light_ds = pipeline.new_descriptor <light> (dpool);

	auto [view_ds, light_ds] = device.update_descriptors(
		DescriptorWritePair { raw_view_ds, view_buf },
		DescriptorWritePair { raw_light_ds, light_buf }
	);

	window.on_drag(MouseButton::Left, [&](double, double, double dx, double dy) {
		const float speed = 0.005f;
		auto yaw = glm::angleAxis(-float(dx * speed), glm::vec3(0.0f, 1.0f, 0.0f));
		auto pitch = glm::angleAxis(float(dy * speed), xform.right());
		xform.rotation = glm::normalize(pitch * yaw * xform.rotation);
	});

	window.set_input_mode(GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

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
		
		// TODO: use transform...
		auto model = glm::mat4(1.0f);
		model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 1.0f));
		
		aperature.aspect = static_cast <float> (extent.width) / extent.height;

		view_buf.write(DataTypeOf <view> {
			.proj = aperature.projection_matrix(),
			.view = xform.view_matrix(),
			.model = model,
		});

		if (window.is_pressed(Key::Escape))
			window.close();

		device.wait_for_frame(frame);
		if (!device.acquire_image_for_frame(frame))
			continue;
		
		auto render_area = vk::Rect2D()
			.setOffset({ 0, 0 })
			.setExtent(frame.extent);

		auto clear_values = std::array <vk::ClearValue, 2> {
			vk::ClearColorValue().setFloat32({ 0.05f, 0.05f, 0.05f, 1.0f }),
			vk::ClearDepthStencilValue(1.0f, 0),
		};

		auto &cmd = cmd_buffers[window.frame_index];
		auto &framebuffer = framebuffers[frame.image_index];

		auto commands = nullptr
			| begin_render_pass(render_pass, framebuffer, render_area, clear_values)
			| bind_pipeline(pipeline)
			| bind_descriptors(view_ds, light_ds)
			| draw_mesh(handle)
			| unbind_pipeline()
			| end_render_pass();

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
