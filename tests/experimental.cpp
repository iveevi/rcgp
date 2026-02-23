#include <array>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <print>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define RCGP_SUPPORT_GLM
#include <rcgp.hpp>

using namespace rcgp;

auto make_model(float elapsed_seconds) -> glm::mat4
{
	auto model = glm::rotate(
		glm::mat4(1.0f),
		elapsed_seconds,
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	return glm::rotate(
		model,
		elapsed_seconds * 0.6f,
		glm::vec3(1.0f, 0.0f, 0.0f)
	);
}

auto make_mvp(const glm::mat4 &model, const vk::Extent2D &extent) -> glm::mat4
{
	const auto aspect = float(extent.width) / float(extent.height);

	auto view = glm::lookAt(
		glm::vec3(0.0f, 0.0f, 5.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f)
	);
	auto projection = glm::perspective(
		glm::radians(60.0f),
		aspect,
		0.1f,
		100.0f
	);
	projection[1][1] *= -1.0f;

	return projection * view * model;
}

AttributeStream <float3> position;
AttributeStream <float3> normal;

using PositionVertex = ResourceTypeFor <position> ::element_type;
using NormalVertex = ResourceTypeFor <normal> ::element_type;

const auto cube_vertices = std::array <PositionVertex, 36> {
	PositionVertex(glm::vec3(-1.0f, -1.0f,  1.0f)),
	PositionVertex(glm::vec3( 1.0f, -1.0f,  1.0f)),
	PositionVertex(glm::vec3( 1.0f,  1.0f,  1.0f)),
	PositionVertex(glm::vec3(-1.0f, -1.0f,  1.0f)),
	PositionVertex(glm::vec3( 1.0f,  1.0f,  1.0f)),
	PositionVertex(glm::vec3(-1.0f,  1.0f,  1.0f)),

	PositionVertex(glm::vec3( 1.0f, -1.0f, -1.0f)),
	PositionVertex(glm::vec3(-1.0f, -1.0f, -1.0f)),
	PositionVertex(glm::vec3(-1.0f,  1.0f, -1.0f)),
	PositionVertex(glm::vec3( 1.0f, -1.0f, -1.0f)),
	PositionVertex(glm::vec3(-1.0f,  1.0f, -1.0f)),
	PositionVertex(glm::vec3( 1.0f,  1.0f, -1.0f)),

	PositionVertex(glm::vec3(-1.0f, -1.0f, -1.0f)),
	PositionVertex(glm::vec3(-1.0f, -1.0f,  1.0f)),
	PositionVertex(glm::vec3(-1.0f,  1.0f,  1.0f)),
	PositionVertex(glm::vec3(-1.0f, -1.0f, -1.0f)),
	PositionVertex(glm::vec3(-1.0f,  1.0f,  1.0f)),
	PositionVertex(glm::vec3(-1.0f,  1.0f, -1.0f)),

	PositionVertex(glm::vec3( 1.0f, -1.0f,  1.0f)),
	PositionVertex(glm::vec3( 1.0f, -1.0f, -1.0f)),
	PositionVertex(glm::vec3( 1.0f,  1.0f, -1.0f)),
	PositionVertex(glm::vec3( 1.0f, -1.0f,  1.0f)),
	PositionVertex(glm::vec3( 1.0f,  1.0f, -1.0f)),
	PositionVertex(glm::vec3( 1.0f,  1.0f,  1.0f)),

	PositionVertex(glm::vec3(-1.0f,  1.0f,  1.0f)),
	PositionVertex(glm::vec3( 1.0f,  1.0f,  1.0f)),
	PositionVertex(glm::vec3( 1.0f,  1.0f, -1.0f)),
	PositionVertex(glm::vec3(-1.0f,  1.0f,  1.0f)),
	PositionVertex(glm::vec3( 1.0f,  1.0f, -1.0f)),
	PositionVertex(glm::vec3(-1.0f,  1.0f, -1.0f)),

	PositionVertex(glm::vec3(-1.0f, -1.0f, -1.0f)),
	PositionVertex(glm::vec3( 1.0f, -1.0f, -1.0f)),
	PositionVertex(glm::vec3( 1.0f, -1.0f,  1.0f)),
	PositionVertex(glm::vec3(-1.0f, -1.0f, -1.0f)),
	PositionVertex(glm::vec3( 1.0f, -1.0f,  1.0f)),
	PositionVertex(glm::vec3(-1.0f, -1.0f,  1.0f)),
};

const auto cube_normals = std::array <NormalVertex, 36> {
	NormalVertex(glm::vec3( 0.0f,  0.0f,  1.0f)),
	NormalVertex(glm::vec3( 0.0f,  0.0f,  1.0f)),
	NormalVertex(glm::vec3( 0.0f,  0.0f,  1.0f)),
	NormalVertex(glm::vec3( 0.0f,  0.0f,  1.0f)),
	NormalVertex(glm::vec3( 0.0f,  0.0f,  1.0f)),
	NormalVertex(glm::vec3( 0.0f,  0.0f,  1.0f)),

	NormalVertex(glm::vec3( 0.0f,  0.0f, -1.0f)),
	NormalVertex(glm::vec3( 0.0f,  0.0f, -1.0f)),
	NormalVertex(glm::vec3( 0.0f,  0.0f, -1.0f)),
	NormalVertex(glm::vec3( 0.0f,  0.0f, -1.0f)),
	NormalVertex(glm::vec3( 0.0f,  0.0f, -1.0f)),
	NormalVertex(glm::vec3( 0.0f,  0.0f, -1.0f)),

	NormalVertex(glm::vec3(-1.0f,  0.0f,  0.0f)),
	NormalVertex(glm::vec3(-1.0f,  0.0f,  0.0f)),
	NormalVertex(glm::vec3(-1.0f,  0.0f,  0.0f)),
	NormalVertex(glm::vec3(-1.0f,  0.0f,  0.0f)),
	NormalVertex(glm::vec3(-1.0f,  0.0f,  0.0f)),
	NormalVertex(glm::vec3(-1.0f,  0.0f,  0.0f)),

	NormalVertex(glm::vec3( 1.0f,  0.0f,  0.0f)),
	NormalVertex(glm::vec3( 1.0f,  0.0f,  0.0f)),
	NormalVertex(glm::vec3( 1.0f,  0.0f,  0.0f)),
	NormalVertex(glm::vec3( 1.0f,  0.0f,  0.0f)),
	NormalVertex(glm::vec3( 1.0f,  0.0f,  0.0f)),
	NormalVertex(glm::vec3( 1.0f,  0.0f,  0.0f)),

	NormalVertex(glm::vec3( 0.0f,  1.0f,  0.0f)),
	NormalVertex(glm::vec3( 0.0f,  1.0f,  0.0f)),
	NormalVertex(glm::vec3( 0.0f,  1.0f,  0.0f)),
	NormalVertex(glm::vec3( 0.0f,  1.0f,  0.0f)),
	NormalVertex(glm::vec3( 0.0f,  1.0f,  0.0f)),
	NormalVertex(glm::vec3( 0.0f,  1.0f,  0.0f)),

	NormalVertex(glm::vec3( 0.0f, -1.0f,  0.0f)),
	NormalVertex(glm::vec3( 0.0f, -1.0f,  0.0f)),
	NormalVertex(glm::vec3( 0.0f, -1.0f,  0.0f)),
	NormalVertex(glm::vec3( 0.0f, -1.0f,  0.0f)),
	NormalVertex(glm::vec3( 0.0f, -1.0f,  0.0f)),
	NormalVertex(glm::vec3( 0.0f, -1.0f,  0.0f)),
};

using CubeIndexBuffer = IndexBuffer <Topology::eTriangleList, uint32_t>;
using CubeIndex = CubeIndexBuffer::element_type;

const auto cube_indices = std::array <CubeIndex, 12> {
	CubeIndex(glm::uvec3(0, 1, 2)),
	CubeIndex(glm::uvec3(3, 4, 5)),
	CubeIndex(glm::uvec3(6, 7, 8)),
	CubeIndex(glm::uvec3(9, 10, 11)),
	CubeIndex(glm::uvec3(12, 13, 14)),
	CubeIndex(glm::uvec3(15, 16, 17)),
	CubeIndex(glm::uvec3(18, 19, 20)),
	CubeIndex(glm::uvec3(21, 22, 23)),
	CubeIndex(glm::uvec3(24, 25, 26)),
	CubeIndex(glm::uvec3(27, 28, 29)),
	CubeIndex(glm::uvec3(30, 31, 32)),
	CubeIndex(glm::uvec3(33, 34, 35)),
};

constexpr auto cube_index_count = uint32_t(cube_indices.size() * 3);

struct View {
	float4x4 model;
	float4x4 mvp;

	$reflection(model, mvp);
};

PushConstant <View> view;

auto vs = $shader(vertex)(
	$contracts(
		(p, position),
		(n, normal),
		(pc, view)
	),
	ClipPosition cpos
) -> float3
{
	cpos = pc.mvp * float4(p, 1.0f);
	return normalize(float3(pc.model * float4(n, 0.0f)));
};

struct Material {
	float3 albedo;

	$reflection(albedo);
};

UniformBuffer <Material> material;

auto fs = $shader(fragment)(
	$contracts(material),
	float3 nrm
)
{
	auto light_dir = normalize(float3(0.35f, 0.75f, 0.55f));
	auto lambert = max(dot(normalize(nrm), light_dir), f32(0.0f));
	auto diffuse = f32(0.18f) + lambert * f32(0.82f);
	return float4(material.albedo * diffuse, 1.0f);
};

int main()
{
	auto [session, dld] = Session::from({});

	auto features = vk::StructureChain <
		vk::PhysicalDeviceVulkan13Features
	> ();

	features.get <vk::PhysicalDeviceVulkan13Features> ()
		.setDynamicRendering(true)
		.setSynchronization2(true);
	
	auto device = Device::from(*session, dld, {
		.extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		},
		.queues = {
			{ "primary", vk::QueueFlagBits::eGraphics }
		},
		.pNext = &features,
	});

	auto window = Window::from(*session, device, {
		.width = 1280,
		.height = 720,
		.title = "Experimental Cube",
	});

	auto queue = device.queues.at("primary");
	auto cpool = CommandPool::from(device, queue);
	auto dpool = DescriptorPool::from(device, {
		.max_sets = 1,
		.uniform_buffers = 1,
	});

	auto comb = RasterizationCombinator <Topology::eTriangleList> {
		.device = device,
		.compiler = ShaderCompiler {},
		.render_state = vk::PipelineRenderingCreateInfo()
			.setColorAttachmentFormats(window.format)
			.setDepthAttachmentFormat(vk::Format::eD32Sfloat),
		.options = RasterizationOptions {
			.extent = {},
			.depth_test = true,
			.cull_mode = vk::CullModeFlagBits::eBack,
			.polygon_mode = vk::PolygonMode::eFill,
			.alpha_blend = false,
		},
	};

	auto pipeline = comb(vs, fs);

	const auto host_visible_coherent =
		vk::MemoryPropertyFlagBits::eHostVisible
		| vk::MemoryPropertyFlagBits::eHostCoherent;

	auto pos_buf = ResourceTypeFor <position> ::from(
		device,
		cube_vertices.size(),
		host_visible_coherent
	);
	pos_buf.write_unsafe(std::span <const PositionVertex> (cube_vertices));

	auto nrm_buf = ResourceTypeFor <normal> ::from(
		device,
		cube_normals.size(),
		host_visible_coherent
	);
	nrm_buf.write_unsafe(std::span <const NormalVertex> (cube_normals));

	auto idx_buf = CubeIndexBuffer::from(
		device,
		cube_indices.size(),
		host_visible_coherent
	);
	idx_buf.write_unsafe(std::span <const CubeIndex> (cube_indices));

	auto mat_buf = ResourceTypeFor <material> ::from(
		device,
		host_visible_coherent
	).write({ .albedo = glm::vec3(0.95, 0.25, 0.18) });

	auto mat_desc = device.update_descriptors(
		DescriptorWrite {
			pipeline.new_descriptor <material> (dpool),
			mat_buf,
		}
	);

	auto view_pc = ResourceTypeFor <view> {};

	auto streams = device.new_command_streams(cpool, window.frames_in_flight);

	Image depth;

	auto resize = [&] {
		device.logical.waitIdle();

		if (depth.handle)
			depth.destroy();

		auto extent = window.extent();
		depth = Image::from(device, {
			.extent = vk::Extent3D(extent.width, extent.height, 1),
			.format = vk::Format::eD32Sfloat,
			.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
			.properties = vk::MemoryPropertyFlagBits::eDeviceLocal,
			.aspect = vk::ImageAspectFlagBits::eDepth,
		});
	};

	resize();

	auto epoch = std::chrono::steady_clock::now();
	while (window.alive()) {
		window.poll();
		if (window.is_pressed(Key::Q))
			window.close();

		auto frame = window.next_frame();

		device.wait_for_frame(frame);
		auto acquire = device.acquire_image_for_frame(window, frame);
		if (acquire == FrameAcquireStatus::OutOfDate)
			continue;

		device.reset_frame_fence(frame);

		auto &stream = streams[window.frame_index];

		stream.begin();

		auto &target = window.images[frame.image_index];
		stream.transition_image_layout(target, vk::ImageLayout::eColorAttachmentOptimal);
		stream.transition_image_layout(depth, vk::ImageLayout::eDepthStencilAttachmentOptimal);

		auto color_attachment = vk::RenderingAttachmentInfo()
			.setImageView(target.view)
			.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eStore)
			.setClearValue(vk::ClearValue(vk::ClearColorValue(
				std::array { 0.04f, 0.06f, 0.10f, 1.0f }
			)));

		auto depth_attachment = vk::RenderingAttachmentInfo()
			.setImageView(depth.view)
			.setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.setLoadOp(vk::AttachmentLoadOp::eClear)
			.setStoreOp(vk::AttachmentStoreOp::eDontCare)
			.setClearValue(vk::ClearValue(vk::ClearDepthStencilValue(1.0f, 0)));

		auto render_area = vk::Rect2D()
			.setOffset(vk::Offset2D(0, 0))
			.setExtent(frame.extent);

		stream.beginRendering(
			vk::RenderingInfo()
				.setRenderArea(render_area)
				.setLayerCount(1)
				.setColorAttachments(color_attachment)
				.setPDepthAttachment(&depth_attachment)
		);

		auto viewport = vk::Viewport()
			.setX(0.0f)
			.setY(0.0f)
			.setWidth(float(frame.extent.width))
			.setHeight(float(frame.extent.height))
			.setMinDepth(0.0f)
			.setMaxDepth(1.0f);

		auto scissor = vk::Rect2D()
			.setOffset(vk::Offset2D(0, 0))
			.setExtent(frame.extent);

		stream.setViewport(0, std::array { viewport });
		stream.setScissor(0, std::array { scissor });

		auto elapsed = std::chrono::duration <float> (
			std::chrono::steady_clock::now() - epoch
		).count();

		auto model = make_model(elapsed);
		view_pc.model = model;
		view_pc.mvp = make_mvp(model, frame.extent);
	
		// TODO: for this to be user friendly we need to really
		// simplify/compress the type signature... especially for
		// scaffolds!
		stream.draw_indexed(pipeline,
			{ mat_desc },
			{ view_pc },
			DrawIndexedParameters { idx_buf, pos_buf, nrm_buf },
			DrawDispatchSize { cube_index_count, }
		);

		stream.endRendering();
		stream.transition_image_layout(target, vk::ImageLayout::ePresentSrcKHR);
		stream.end();

		queue.submit(
			{ stream },
			{ frame.presented },
			{ window.image_rendered[frame.image_index] },
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			frame.fence
		);

		auto present = queue.present(
			window,
			frame.image_index,
			{ window.image_rendered[frame.image_index] }
		);

		if (present != vk::Result::eSuccess
			&& present != vk::Result::eSuboptimalKHR
			&& present != vk::Result::eErrorOutOfDateKHR) {
			std::println(std::cerr, "presentKHR failed ({})", vk::to_string(present));
			std::abort();
		}
	}
}
