#include <mrd.hpp>

#include <ugp.hpp>

#include "util/aperature.hpp"
#include "util/transform.hpp"

constexpr mrd::MeshEncodings mesh_encodings;
using Model = mrd::Model <mesh_encodings>;
using Mesh = Model::mesh_t;

constexpr auto albedo_encoding = mrd::TextureEncoding::RGBA_UNorm8;
using Texture = mrd::Texture <albedo_encoding>;

struct TextureGPU {
	Image image;
	vk::Sampler sampler;

	void destroy(const Device &device) {
		if (sampler) device.logical.destroySampler(sampler);
		image.destroy();
	}
};

// TODO: use this newline convention
static TextureGPU upload_texture(
	const Device &device,
	const Queue &queue,
	const CommandPool &cpool,
	const Texture &tex
)
{
	// create staging buffer
	const auto bytes = tex.data.size() * sizeof(tex.data[0]);
	auto staging = Buffer::from(
		device,
		bytes,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent
	);

	staging.write(tex.data.data(), bytes, 0);

	// GPU image
	auto img_info = Image::Info {
		.extent = vk::Extent2D(tex.info.size.x, tex.info.size.y),
		// .format = vk::Format::eR32G32B32A32Sfloat,
		.format = vk::Format::eR8G8B8A8Unorm,
		.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
		.properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
		.tiling = vk::ImageTiling::eOptimal,
		.aspect = vk::ImageAspectFlagBits::eColor,
	};

	auto image = Image::from(device, img_info);

	// one-shot command buffer
	auto cmd = device.new_command_buffers(cpool, 1).front();
	cmd.begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

	auto barrier1 = image.memory_barrier(
		vk::ImageLayout::eTransferDstOptimal,
		{}, vk::AccessFlagBits::eTransferWrite
	);
	cmd.pipelineBarrier(
		vk::PipelineStageFlagBits::eTopOfPipe,
		vk::PipelineStageFlagBits::eTransfer,
		{}, nullptr, nullptr, barrier1
	);

	auto copy = vk::BufferImageCopy()
		.setImageSubresource(
			vk::ImageSubresourceLayers()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setMipLevel(0)
				.setBaseArrayLayer(0)
				.setLayerCount(1)
		)
		.setImageExtent(vk::Extent3D(tex.info.size.x, tex.info.size.y, 1));
	cmd.copyBufferToImage(staging.handle, image.handle, vk::ImageLayout::eTransferDstOptimal, copy);

	auto barrier2 = image.memory_barrier(
		vk::ImageLayout::eShaderReadOnlyOptimal,
		vk::AccessFlagBits::eTransferWrite,
		vk::AccessFlagBits::eShaderRead
	);
	cmd.pipelineBarrier(
		vk::PipelineStageFlagBits::eTransfer,
		vk::PipelineStageFlagBits::eFragmentShader,
		{}, nullptr, nullptr, barrier2
	);

	cmd.end();
	queue.submit(cmd, {}, {}, vk::PipelineStageFlagBits::eTopOfPipe, nullptr);
	queue.waitIdle();

	staging.destroy();

	auto sinfo = vk::SamplerCreateInfo()
		.setMagFilter(vk::Filter::eLinear)
		.setMinFilter(vk::Filter::eLinear)
		.setAddressModeU(vk::SamplerAddressMode::eRepeat)
		.setAddressModeV(vk::SamplerAddressMode::eRepeat)
		.setAddressModeW(vk::SamplerAddressMode::eRepeat)
		.setAnisotropyEnable(false)
		.setUnnormalizedCoordinates(false);

	TextureGPU gpu;
	gpu.image = image;
	gpu.sampler = device.logical.createSampler(sinfo);
	return gpu;
}

struct View {
	mat4 proj;
	mat4 view;
	mat4 model;

	$reflection(proj, view, model);
};

AttributeStream <vec3> position;
AttributeStream <vec3> normal;
AttributeStream <vec2> texcoord;

UniformBuffer <View> view;
Sampler2D albedo_tex;

auto vs = $vertex $fn($ref(position), $ref(normal), $ref(texcoord), $ref(view))
	-> $returns(Position, Smooth <vec3>, Smooth <vec2>)
{
	auto mvp = view.proj * view.view * view.model;
	auto nmat = transpose(inverse(mat3(view.model)));
	auto wn = normalize(mat3(view.model) * normal);
	auto pp = mvp * vec4(position, 1.0);
	$return std::tuple(pp, wn, texcoord);
};

// TODO: push constants
UniformBuffer <vec3> light;

auto fs = $fragment $fn($ref(light), $ref(albedo_tex), vec3 world_normal, vec2 uv) -> $returns(vec4)
{
	auto normal = normalize(world_normal);
	auto shade = max(dot(light, normal), 0.0f);
	vec4 texel = albedo_tex.sample(uv);
	// $return vec4(texel.xyz * shade, 1.0f);
	// $return vec4(vec3(texel) * shade, 1.0f);
	$return vec4(vec3(texel), 1.0f);
};

using TriangleBuffer = IndexBuffer <Topology::eTriangleList, uint32_t>;

struct MeshHandle {
	size_t draw_count;
	glm::mat4 transform { 1.0f };
	
	TriangleBuffer triangles;
	VertexBufferOf <position> positions;
	VertexBufferOf <normal> normals;
	VertexBufferOf <texcoord> uvs;
	ResourceMirrorOf <view> view_buf;
	DescriptorOf <view> view_ds;

	static auto from(const Device &device, const Mesh &mesh) {
		MeshHandle result;

		result.draw_count = 3 * mesh.primitives.size();
		result.transform = mesh.transform;

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

		if constexpr (mesh_encodings.uvs != mrd::R2::Disable) {
			result.uvs = VertexBufferOf <texcoord> ::from(
				device, mesh.uvs.size(),
				vk::MemoryPropertyFlagBits::eHostVisible
				| vk::MemoryPropertyFlagBits::eHostCoherent
			).write(span(mesh.uvs).map(
				[](auto p) -> DynamicDataTypeOf <texcoord> {
					return { p.x, p.y };
				}
			));
		}

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
		| bind_vertex_buffers(handles.positions, handles.normals, handles.uvs)
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
		.combined_image_samplers = 1 << 10,
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

	std::vector <MeshHandle> handles;
	handles.reserve(model.meshes.size());

	for (auto &mesh : model.meshes) {
		mesh.recalculate_normals();
		handles.push_back(MeshHandle::from(device, mesh));
	}

	// Camera
	Aperature aperature;
	Transform xform;

	xform.translation = glm::vec3(0, 0, -10);
	aperature.aspect = float(window.extent().width) / window.extent().height;

	auto light_buf = ResourceMirrorOf <light> ::from(
		device,
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent
	).write(glm::vec3(0, 1, 0));
	
	// Upload albedo textures per material (fallback to material color or magenta) and descriptor sets
	auto make_color_texture = [](glm::vec4 color) {
		Texture tex;
		tex.info.size = { 1, 1 };
		tex.data.emplace_back(color.r, color.g, color.b, color.a);
		return tex;
	};

	const glm::vec4 fallback_color { 1.0f, 0.0f, 1.0f, 1.0f };

	std::vector <TextureGPU> material_gpu_textures;
	std::vector <ResourceMirrorOf <albedo_tex>> material_albedo_mirrors;
	std::vector <DescriptorOf <albedo_tex>> material_albedo_ds;

	if (model.materials.empty())
		model.materials.push_back(mrd::Material{});

	material_gpu_textures.reserve(model.materials.size());
	material_albedo_mirrors.reserve(model.materials.size());
	material_albedo_ds.reserve(model.materials.size());

	for (const auto &mat : model.materials) {
		Texture src;

		if (mat.albedo.has_texture()) {
			static std::optional <mrd::TextureCache <albedo_encoding>> albedo_cache;
			if (!albedo_cache) albedo_cache.emplace();
			auto loaded = Texture::load(mat.albedo.path, *albedo_cache);
			if (loaded) {
				src = std::move(*loaded);
			} else {
				src = make_color_texture(fallback_color);
				error("failed to load image: {}", loaded.error().message);
			}
		} else {
			src = make_color_texture(mat.albedo.value);
		}

		material_gpu_textures.push_back(upload_texture(device, queue, cpool, src));

		ResourceMirrorOf <albedo_tex> mirror;
		mirror.sampler = material_gpu_textures.back().sampler;
		mirror.view = material_gpu_textures.back().image.view;
		mirror.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
		material_albedo_mirrors.push_back(mirror);

		auto raw_albedo_ds = pipeline.new_descriptor <albedo_tex> (dpool);
		auto ds = device.update_descriptors(
			DescriptorWritePair { raw_albedo_ds, material_albedo_mirrors.back() }
		);

		material_albedo_ds.push_back(ds);
	}

	auto raw_light_ds = pipeline.new_descriptor <light> (dpool);

	auto light_ds = device.update_descriptors(
		DescriptorWritePair { raw_light_ds, light_buf }
	);

	for (auto &handle : handles) {
		handle.view_buf = ResourceMirrorOf <view> ::from(
			device,
			vk::MemoryPropertyFlagBits::eHostVisible
			| vk::MemoryPropertyFlagBits::eHostCoherent
		).write(DataTypeOf <view> {
			.proj = aperature.projection_matrix(),
			.view = xform.view_matrix(),
			.model = handle.transform,
		});

		auto raw_view_ds = pipeline.new_descriptor <view> (dpool);
		handle.view_ds = device.update_descriptors(
			DescriptorWritePair { raw_view_ds, handle.view_buf }
		);
	}

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
		aperature.aspect = static_cast <float> (extent.width) / extent.height;

		for (auto &handle : handles) {
			handle.view_buf.write(DataTypeOf <view> {
				.proj = aperature.projection_matrix(),
				.view = xform.view_matrix(),
				.model = handle.transform,
			});
		}

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
			| bind_pipeline(pipeline);

		for (size_t i = 0; i < handles.size(); ++i) {
			commands = commands
				| bind_descriptors(
					handles[i].view_ds,
					light_ds,
					material_albedo_ds[
						i < model.mesh_material_indices.size()
							? std::min <size_t> (model.mesh_material_indices[i], material_albedo_ds.size() - 1)
							: 0
					]
				)
				| draw_mesh(handles[i]);
		}

		auto final_commands = commands
			| unbind_pipeline()
			| end_render_pass();

		cmd = final_commands(cmd);
		queue.submit(cmd,
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

	device.logical.waitIdle();
	for (auto &g : material_gpu_textures)
		g.destroy(device);
}
