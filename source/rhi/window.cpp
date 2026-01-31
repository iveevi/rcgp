#include "rhi/window.hpp"

#include <utility>

namespace rcgp {

struct HandlerTable {
	int dragging_button = -1;
	bool cursor_initialized = false;
	double last_x = 0.0;
	double last_y = 0.0;

	std::vector <MouseButtonHandler> mouse_button;
	std::vector <CursorMoveHandler> cursor_move;
	std::unordered_map <int, std::vector <DragHandler>> drag;
};

inline std::vector <HandlerTable> handler_tables;

void dispatch_mouse_button(GLFWwindow *w, int button, int action, int mods)
{
	auto user = glfwGetWindowUserPointer(w);
	auto handler_index = reinterpret_cast <std::intptr_t> (user);

	auto &ht = handler_tables[handler_index];
	for (auto &cb : ht.mouse_button)
		cb(button, action, mods);

	if (action == GLFW_PRESS) {
		ht.dragging_button = button;
		glfwGetCursorPos(w, &ht.last_x, &ht.last_y);
		ht.cursor_initialized = true;
	} else if (action == GLFW_RELEASE) {
		if (ht.dragging_button == button)
			ht.dragging_button = -1;
	}
}

void dispatch_cursor_pos(GLFWwindow *w, double xpos, double ypos)
{
	auto user = glfwGetWindowUserPointer(w);
	auto handler_index = reinterpret_cast <std::intptr_t> (user);

	auto &ht = handler_tables[handler_index];

	double dx = 0.0;
	double dy = 0.0;
	if (ht.cursor_initialized) {
		dx = xpos - ht.last_x;
		dy = ypos - ht.last_y;
	}

	ht.last_x = xpos;
	ht.last_y = ypos;
	ht.cursor_initialized = true;
	for (auto &cb : ht.cursor_move)
		cb(xpos, ypos, dx, dy);

	if (ht.dragging_button != -1) {
		auto it = ht.drag.find(ht.dragging_button);
		if (it != ht.drag.end()) {
			for (auto &cb : it->second)
				cb(xpos, ypos, dx, dy);
		}
	}
}

void Window::poll() const
{
	glfwPollEvents();
}

void Window::close() const
{
	glfwSetWindowShouldClose(handle, true);
}

bool Window::alive() const
{
	return not glfwWindowShouldClose(handle);
}

bool Window::is_pressed(Key key) const
{
	return glfwGetKey(handle, std::to_underlying(key)) == GLFW_PRESS;
}

void Window::set_input_mode(int mode, int value) const
{
	glfwSetInputMode(handle, mode, value);
}

vk::Extent2D Window::extent() const
{
	int width;
	int height;

	glfwGetFramebufferSize(handle, &width, &height);

	return vk::Extent2D(width, height);
}

Frame Window::next_frame()
{
	frame_index = (frame_index + 1) % frames_in_flight;
	auto &frame = frames[frame_index];
	frame.extent = extent();
	return frame;
}

void Window::on_mouse_button(MouseButtonHandler handler)
{
	auto &ht = handler_tables[handler_index];
	ht.mouse_button.push_back(std::move(handler));
}

void Window::on_cursor_move(CursorMoveHandler handler)
{
	auto &ht = handler_tables[handler_index];
	ht.cursor_move.push_back(std::move(handler));
}

void Window::on_drag(MouseButton button, DragHandler handler)
{
	auto idx = std::to_underlying(button);
	auto &ht = handler_tables[handler_index];
	ht.drag[idx].push_back(std::move(handler));
}

Window Window::from(const Session &session, const Device &device, const Options &options)
{
	auto &ldev = device.logical;
	auto &pdev = device.physical;

	Window result;

	result.handle = glfwCreateWindow(options.width, options.height, options.title, nullptr, nullptr);
	result.handler_index = handler_tables.size();
	handler_tables.emplace_back();

	auto user = reinterpret_cast <void *> (result.handler_index);
	glfwSetWindowUserPointer(result.handle, user);
	glfwSetMouseButtonCallback(result.handle, dispatch_mouse_button);
	glfwSetCursorPosCallback(result.handle, dispatch_cursor_pos);

	VkSurfaceKHR surface;
	glfwCreateWindowSurface(session.handle, result.handle, nullptr, &surface);
	result.surface = surface;

	result.format = vk::Format::eB8G8R8A8Unorm;

	auto surface_capabilities = pdev.getSurfaceCapabilitiesKHR(surface);

	auto swapchain_info = vk::SwapchainCreateInfoKHR()
		.setImageArrayLayers(1)
		.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
		.setImageExtent(result.extent())
		.setImageFormat(result.format)
		.setMinImageCount(surface_capabilities.minImageCount)
		.setOldSwapchain(nullptr)
		.setPresentMode(options.present_mode)
		.setImageUsage(
			vk::ImageUsageFlagBits::eColorAttachment
			| vk::ImageUsageFlagBits::eTransferDst
		)
		.setSurface(surface);

	result.swapchain = ldev.createSwapchainKHR(swapchain_info);

	auto swapchain_images = ldev.getSwapchainImagesKHR(result.swapchain);

	result.images.reserve(swapchain_images.size());
	for (auto &handle : swapchain_images) {
		Image image;
		image.device = device.logical;
		image.handle = handle;
		image.layout = vk::ImageLayout::ePresentSrcKHR;
		image.description.extent = vk::Extent3D(result.extent(), 1);
		image.description.format = result.format;
		image.description.aspect = vk::ImageAspectFlagBits::eColor;

		auto view_info = vk::ImageViewCreateInfo()
			.setImage(handle)
			.setViewType(vk::ImageViewType::e2D)
			.setSubresourceRange(
				vk::ImageSubresourceRange()
					.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setBaseArrayLayer(0)
					.setBaseMipLevel(0)
					.setLayerCount(1)
					.setLevelCount(1)
			)
			.setFormat(result.format);

		image.view = ldev.createImageView(view_info);

		result.images.push_back(image);
	}

	result.frame_index = 0;
	result.frames_in_flight = result.images.size();

	auto fence_info = vk::FenceCreateInfo()
		.setFlags(vk::FenceCreateFlagBits::eSignaled);

	auto semaphore_info = vk::SemaphoreCreateInfo();

	result.frames.resize(result.frames_in_flight);
	for (auto &frame : result.frames) {
		frame.swapchain = result.swapchain;
		frame.fence = ldev.createFence(fence_info);
		frame.rendered = ldev.createSemaphore(semaphore_info);
		frame.presented = ldev.createSemaphore(semaphore_info);
	}

	return result;
}

} // namespace rcgp
