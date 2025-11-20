#include <ugp.hpp>

#include "util/transform.hpp"

// TODO: mesh/model stuff goes into ext/
struct Mesh {};

// TODO: first parameter should be alignment rules...
// TODO: for now we are assuming GLSL alignment rules...
// NOTE: TBuffer is typed by its elements as if it
// were an aggregate with the fields Ts... Following
// Vulkan/GLSL rules, all but the last field must be
// trivially constructable; the last field may be an
// unsized array indicated by T[]

// In general, follow this convention:
// X_layout_engine <uint32_t, glm::vec3[]> ::type
// = sequence <
// 	offset_by <uint32_t, 0>,
// 	offset_by <glm::vec3[], 16>
// >

// TODO: no more fields after dynamic part
template <typename T, size_t Offset>
struct offset_by {};

template <typename ... Ts>
struct std430_layout_engine {};
// TODO: next and type

// TODO: LayoutMappedBuffer <layout T>

template <typename ... Ts>
struct FieldedBuffer {}; // : LayoutMappedBuffer <X_layout_engine <Ts...> ::type> {}

// template <typename ... Ts>
// struct Buffer : Buffer {
// 	// TODO: method to write individual fields at a time
// 	// TODO: alternatively provide a host "staging" item
// 	// where unsized arrays are converted to vectors or so...
// };

// using PointsBuffer = TBuffer <
// 	uint32_t, 	// count
// 	glm::vec3[]	// positions
// >;

// NOTE: so the flow from DSL storage buffer to host handle;
// take the expanded reflection, convert the fields into
// host equivalents (ordinary + array), then compute their
// layout, and serve the TBuffer of that layout...
// AND
// serve a host staging/data prep structure; the base type
// will be a tuple, but then use the scaffold field members
// to indirectly do this (with overloaded op=)

enum class CommandsPhase {
	Begin,
	End,
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
	// auto compiler = Compiler::from(device, Compiler::Info());
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
