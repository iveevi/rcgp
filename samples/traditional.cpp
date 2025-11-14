#include <ugp.hpp>

struct RasterForward {
	Position svpos;
	Smooth <vec3> position;

	$reflection(svpos, position);
};

struct MVP {
	mat4 model;
	mat4 view;
	mat4 proj;

	$reflection(model, view, proj);
};

ParameterBlock <MVP> mvp;

// auto vs = $vertex $fn($use(mvp), vec3 position, vec3 normal, vec2 uv) -> $returns(RasterForward)
// {
// 	vec4 hpos = vec4(position, 1);
// 	vec4 ppos = mvp.model * hpos;
//
// 	$return RasterForward {
// 		.svpos = mvp.proj * mvp.view * ppos,
// 		// .position = ppos.xyz,
// 		.position = vec3(),
// 	};
// };
//
// auto fs = $fragment $fn(vec3 position) -> $returns(SubpassResult <vec4>)
// {
// 	vec3 dU = dFdx(position);
// 	vec3 dV = dFdy(position);
// 	vec3 N = normalize(cross(dU, dV));
// 	$return vec4(0.5f + 0.5f * N, 1.0);
// };

// geometry buffers := index & vertex buffer is effectively
// a map from primitive index -> primitive data
// TODO: geometry streaming, geometry transformation, and shading stages later on...
struct Vertex {
	vec3 position;
	vec3 normal;
	vec2 uv;

	// TODO: alignment is specified as a static constant?
	// static auto alignment_policy = AlignmentPolicy::GLSL430/Scalar;
	// OR macrofiy with
	// $align(GLSL430); or $align(Scalar);

	$reflection(position, normal, uv);
};

// TODO: need to overwrite to the host mirror...
#define $solid(T) T

// TODO: mesh/model stuff goes into ext/
struct Mesh {};

struct Buffer {};

template <typename T>
struct TBuffer : Buffer {};

struct Transform {
	// glm::vec3 translation;
	// glm::vec3 scaling;
};

struct GeometryBuffer {
	TBuffer <ivec3> triangles;
	TBuffer <Vertex> vertices;
	Transform xform;

	static GeometryBuffer from(const Device &, const Mesh &) {
		GeometryBuffer result;

		// TODO: fdsfdsf...

		// TODO: alignment_policy and field generators...
		auto v = $solid(Vertex) {
			.position = vec3(),
			.normal = vec3(),
			.uv = vec2(),
		};

		return result;
	};
};

// TODO: material stuff...

enum class CommandsPhase {
	Begin,
	End,
};

template <typename T>
struct List : std::vector <T> {
	using std::vector <T> ::vector;

	// TODO: map
};

struct Window {
	GLFWwindow *handle;
	vk::SurfaceKHR surface;

	vk::Format format;
	vk::SwapchainKHR swapchain;
	List <vk::ImageView> views;

	struct Frame {
		vk::Fence fence;
		vk::Semaphore presented;
		vk::Semaphore rendered;
		vk::Extent2D extent;
		uint32_t image_index;
	};

	List <Frame> frames;

	size_t frames_in_flight;
	size_t frame_index;

	bool alive() const {
		return not glfwWindowShouldClose(handle);
	}

	void poll() const {
		glfwPollEvents();
	}

	vk::Extent2D extent() const {
		int width;
		int height;

		glfwGetFramebufferSize(handle, &width, &height);

		return vk::Extent2D(width, height);
	}

	Frame next_frame() {
		frame_index = (frame_index + 1) % frames_in_flight;
		auto &frame = frames[frame_index];
		frame.extent = extent();
		return frame;
	}

	// TODO: callback methods...

	// TODO: info struct...
	static Window from(const Session &session, const Device &device) {
		auto &ldev = device.logical;
		auto &pdev = device.physical;

		Window result;

		// GLFW window handle creation
		glfwInit();
		result.handle = glfwCreateWindow(1024, 1024, "ugp", nullptr, nullptr);

		// Surface creation
		VkSurfaceKHR surface;
		glfwCreateWindowSurface(session.handle, result.handle, nullptr, &surface);
		result.surface = surface;

		// Swapchain creation
		result.format = vk::Format::eB8G8R8A8Unorm;

		auto surface_capabilities = pdev.getSurfaceCapabilitiesKHR(surface);

		// TODO: usage should also be in info
		auto swapchain_info = vk::SwapchainCreateInfoKHR()
			.setImageArrayLayers(1)
			.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
			.setImageExtent(result.extent())
			.setImageFormat(result.format)
			.setMinImageCount(surface_capabilities.minImageCount)
			.setOldSwapchain(nullptr)
			.setPresentMode(vk::PresentModeKHR::eFifo)
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
			.setSurface(surface);

		result.swapchain = ldev.createSwapchainKHR(swapchain_info);

		// Prepare images
		auto images = ldev.getSwapchainImagesKHR(result.swapchain);

		result.views.reserve(images.size());
		for (auto &image : images) {
			auto range = vk::ImageSubresourceRange()
				.setAspectMask(vk::ImageAspectFlagBits::eColor)
				.setBaseArrayLayer(0)
				.setBaseMipLevel(0)
				.setLayerCount(1)
				.setLevelCount(1);

			auto view_info = vk::ImageViewCreateInfo()
				.setImage(image)
				.setViewType(vk::ImageViewType::e2D)
				.setSubresourceRange(range)
				.setFormat(result.format);

			result.views.push_back(ldev.createImageView(view_info));
		}

		result.frame_index = 0;
		result.frames_in_flight = images.size();

		// Allocate synchronization information
		auto fence_info = vk::FenceCreateInfo()
			.setFlags(vk::FenceCreateFlagBits::eSignaled);

		auto semaphore_info = vk::SemaphoreCreateInfo();

		result.frames.resize(result.frames_in_flight);
		for (auto &frame : result.frames) {
			frame.fence = ldev.createFence(fence_info);
			frame.rendered = ldev.createSemaphore(semaphore_info);
			frame.presented = ldev.createSemaphore(semaphore_info);
		}

		return result;
	}
};

struct group_device_window {
	Device &device;
	Window &window;

	void wait(Window::Frame &frame, uint64_t timeout = UINT64_MAX) {
		auto result = device.logical.waitForFences(frame.fence, true, timeout);
		device.logical.resetFences(frame.fence);
	}

	bool acquire_image(Window::Frame &frame, uint64_t timeout = UINT64_MAX) {
		auto result = device.logical.acquireNextImageKHR(
			window.swapchain,
			timeout,
			frame.presented,
			nullptr,
			&frame.image_index
		);

		return !(result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR);
	}
};

auto group(Device &device, Window &window)
{
	return group_device_window(device, window);
}

struct Queue : vk::Queue {
	uint32_t family_index;
	uint32_t queue_index;

	static Queue from(const Device &device) {
		// TODO: queue info struct with family flags..
		Queue result(device.logical.getQueue(0, 0));
		result.family_index = 0;
		result.queue_index = 0;
		return result;
	}
};

struct CommandPool : vk::CommandPool {
	static CommandPool from(const Device &device, const Queue &queue) {
		auto cpool_info = vk::CommandPoolCreateInfo()
			.setQueueFamilyIndex(queue.family_index)
			.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

		return CommandPool(device.logical.createCommandPool(cpool_info));
	}
};

struct group_device_cpool {
	const Device &device;
	const CommandPool &cpool;

	auto allocate(uint32_t count) {
		return device.logical.allocateCommandBuffers(
			vk::CommandBufferAllocateInfo()
				.setCommandBufferCount(count)
				.setCommandPool(cpool)
		);
	}
};

auto group(const Device &device, const CommandPool &cpool)
{
	return group_device_cpool(device, cpool);
}

// TODO: bind abstraction again...?

// // Sketching a command buffer eDSL
// struct cmd_ppl_dispatch_ready {
// 	// represents () -> Cmd[X]
// 	static auto inline canonical = []() {};
//
// 	// NOTE: the result of this is an operator Cmd[Ppl] -> Cmd[Ppl]
// 	// TODO: templates, etc.
// 	static auto inline draw_operator = [](/* combinator parameters ... */) {
// 	};
// };
//
// NOTE: the $cmd operator redirects the $fn to make it a host runtime thing, unlike shaders
// auto draw_geobuf = $cmd(cmd_ppl_zero, cmd_ppl_zero) $fn(GeometryBuffer geobuf, Camera camera) {
// 	/* expands to something ... decltype(cmd_ppl_zero.canonical) origin, */
// 	// TODO: should be able to use real control flow
// 	
// 	auto mvp = $solid(MVP) {
// 		.model = geobuf.xform.matrix(),
// 		.view = camera.view_matrix(),
// 		.proj = camera.proj_matrix(),
// 	};
//
// 	mvp_handle.write(mvp);
//
//	auto a = $origin;
// 	auto b = a.bind_resource(mvp_handle);
// 	auto c = b.bind_vertex_buffer(...);
//
// 	// equivalent to: a = draw_indexed(...)(c);
// 	return c.draw_indexed(...);
// };
//
// TODO: should add an option in $cmd to nudge it torwards a secondary command buffer...
// note that this would need a separate command buffer; so this is a two stage thing
// with representational secondary cmd and then secondary cmd, or something
//
// NOTE: if there are no parameters (i.e. $fn()) or if the arguments are detected to be identical
// then we can store the result into a secondary command buffer and reuse it...
//
// NOTE: cmd_neutral = cmd_begin
// auto geobuf_renderpass = $cmd(cmd_neutral, cmd_neutral) $fn(std::vector <GeometryBuffer> &geobufs, Camera camera) {
// 	auto a = $origin;
//
// TODO: how do we actually get origin when its in $cmd?
//
// 	auto b = origin.bind_pipeline(ppl);
//
// 	for (auto &geobuf : geobufs) {
// 		// cmd.execute(<unit>, <args...>)
// 		/* auto c = */ b.execute(draw_geobuf, geobuf, camera);
// 		// OR
// 		/* auto c = */ draw_geobuf(geobuf, camera)(b);
// 		// which treats draw_geobuf as a compositor (which it kind of is?)
//
// 		// ans: draw_geobuf is a stage
// 		// the composition of b with draw_geobuf
// 		// results in a new stage...
// 		// in that sense, execute is a compositor!
// 	}
//
// 	return a;
// };
//
// NOTE: its safe to return a here directly since there is no
// unbindle pipeline command; however, for render passes,
// the user would need to explicitly end the render pass to
// return to the neutral state -- this is probably where
// having $renderpass scopes would be nice, but regardless...
// the fact that the user can arbitrary return when its not
// necessarily sequenced is problamatic... we need something stricter
//
// NOTE: Also should allow some states to 'decay' into others; pipeline[X]
// should be able to decay into pipeline[Neutral], etc.
//
// This may likely be fixed by forced all scopable/non-decaying operations
// into a blocking form like its own control flow; then every `auto x = ...`
// is necessarily some decay-safe type?
//
// Example: $renderpass(x, $origin, args...) { ... }
//
// TODO: check if this is foolproof
//
// TODO: how does this fare against compositor logic and basic lambdas?
//
// I suppose you would have to otherwise pass the typestated command buffer
// into the lambda as well... but is that really working with stages?
// and is there a strict need to work with stages or is the typestate
// alone sufficient
//
// TODO: better syntactic sugar?
//
// TODO: command buffers are purely sequential... may have some way of
// taking advantage of this... should we use $for and other overrided
// control flow?
//
// Strongest way to ensure this is to use the piping form...
// 
// cmd
// | Begin()
// | BeginRenderPass(...)
// | BindPipeline(...)
// | $for(auto &geobuf : geobufs) {
// 	auto mvp = $solid(MVP) {
// 		.model = geobuf.xform.matrix(),
// 		.view = camera.view_matrix(),
// 		.proj = camera.proj_matrix(),
// 	};
//
// 	mvp_handle.write(mvp);
//
// 	auto b = a.bind_resource(mvp_handle);
// 	auto c = b.bind_vertex_buffer(...);
//
// 	// equivalent to: a = draw_indexed(...)(c);
// 	a = c.draw_indexed(...);
// }
//
// NOTE: actually just working with morhpisms Work[X, Y]: Cmd[X] -> Cmd[Y]
// is sufficient, as every instance is valid...
//
// A complete command buffer is Work[Neutral, Neutral]
//
// Fully inline:
//
// auto a = begin_render_pass(...);
// auto b = bind_pipeline(...);
//
// for (auto &geobuf : geobufs) {
// 	auto mvp = ...;
// 	mvp_handle.write(mvp);
// 	auto c = bind_resource(mvp_handle);
// 	auto d = bind_vertex_buffer(...);
// 	auto e = draw_indexed(...);
// 	b |= seq(c, d, e); // b = seq(b, c, d, e);
// }
//
// auto f = end_render_pass(...);
//
// auto work = seq(a, b, f);
//
// queue.submit(work...);
//
// NOTE: seq is the stage combinator for this stuff...
//
// NOTE: should allow out of order plugging in:
// in other words, this all has to be representational as well,
// with the exception that parts of it can be host ctrl flow
// this might have an impact on performance if we need to store
// constants all the time, but oh well
//
// To establish secondary command buffers we can just do:
// auto cached_work = group(device, cpool).as_secondary(work);
// if X and Y are appropriate...

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
	auto &ldev = device.logical;

	auto window = Window::from(session, device);

	// Are compiler's combinators?
	// TODO: should allow one/arbitrary argument combinators?
	auto compiler = Compiler::from(device, Compiler::Info());
	// auto vsm = compiler(vs);
	// auto fsm = compiler(fs);
	//
	// auto tgpc = combinators::tgp <Triangle> (Culling::None);
	// auto ppl = tpgc(vsm, fsm);
	//
	// auto mvp_handle = ppl.handle <mvp> ();

	// // Model : List <Mesh>
	// auto model = Model::load("resources/meshes/armadillo.obj");
	//
	// // List::map
	// auto geobufs = model.map([&](auto &mesh)
	// {
	// 	return GeometryBuffer::from(device, mesh);
	// });

	auto queue = Queue::from(device);
	auto cpool = CommandPool::from(device, queue);
	auto cmdbuffers = group(device, cpool).allocate(window.frames_in_flight);

	// auto renderpass = RenderPass::from();
	// auto framebuffers = List <Framebuffer> ::from();
	//
	// auto camera = Camera::from();

	while (window.alive()) {
		window.poll();

		if (glfwGetKey(window.handle, GLFW_KEY_Q) == GLFW_PRESS)
			glfwSetWindowShouldClose(window.handle, true);

		auto frame = window.next_frame();

		group(device, window).wait(frame);
		group(device, window).acquire_image(frame);

		auto &cmd = cmdbuffers[window.frame_index];

		cmd.begin(vk::CommandBufferBeginInfo());
		cmd.end();

		vk::PipelineStageFlags wait_stage = vk::PipelineStageFlagBits::eAllCommands;

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

		// auto result = ldev.waitForFences(next.fence, true, UINT64_MAX, dld);

		// ...

		// ldev.resetFences(next.fence);
		
		// Each map Cmd[X] -> Cmd[Y] is a stage?
		// compositors are then fairly natural...
		//
		// EDSL:
		//
		// $stage(Frame) $cmd(...) { ... }

		// auto next = window.next_frame_info();
		// next.wait();
		//
		// auto &fbuf = framebuffers[next.image_index];
		//
		// auto origin = cmdbuffers[next.frame_index];
		//
		// // origin: Commands <Renderpass>
		// auto a = origin.bind_pipeline(ppl);
		// for (auto &geobuf : geobufs) {
		// 	auto mvp = $solid(MVP) {
		// 		.model = geobuf.xform.matrix(),
		// 		.view = camera.view_matrix(),
		// 		.proj = camera.proj_matrix(),
		// 	};
		//
		// 	mvp_handle.write(mvp);
		//
		// 	auto b = a.bind_resource(mvp_handle);
		// 	auto c = b.bind_vertex_buffer(...);
		//
		// 	// equivalent to: a = draw_indexed(...)(c);
		// 	a = c.draw_indexed(...);
		// }
		//
		// a.unbind_pipeline();
		//
		// // TODO: also synchronization..
		// // mark read only usage with (const $use(X), ...)
		//
		// queue.submit();
	}
}
