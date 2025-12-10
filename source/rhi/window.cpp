#include "rhi/window.hpp"

#include <utility>

namespace {

void dispatch_mouse_button(GLFWwindow *w, int button, int action, int mods)
{
	auto *self = static_cast <Window *> (glfwGetWindowUserPointer(w));
	if (!self)
		return;
	for (auto &cb : self->mouse_button_handlers)
		cb(button, action, mods);

	if (action == GLFW_PRESS) {
		self->dragging_button = button;
		glfwGetCursorPos(w, &self->last_x, &self->last_y);
		self->cursor_initialized = true;
	} else if (action == GLFW_RELEASE) {
		if (self->dragging_button == button)
			self->dragging_button = -1;
	}
}

void dispatch_cursor_pos(GLFWwindow *w, double xpos, double ypos)
{
	auto *self = static_cast <Window *> (glfwGetWindowUserPointer(w));
	if (!self)
		return;
	double dx = 0.0;
	double dy = 0.0;
	if (self->cursor_initialized) {
		dx = xpos - self->last_x;
		dy = ypos - self->last_y;
	}
	self->last_x = xpos;
	self->last_y = ypos;
	self->cursor_initialized = true;

	for (auto &cb : self->cursor_move_handlers)
		cb(xpos, ypos, dx, dy);
	if (self->dragging_button != -1) {
		auto it = self->drag_handlers.find(self->dragging_button);
		if (it != self->drag_handlers.end()) {
			for (auto &cb : it->second)
				cb(xpos, ypos, dx, dy);
		}
	}
}

} // namespace

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
	return glfwGetKey(handle, static_cast<int>(key)) == GLFW_PRESS;
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
	mouse_button_handlers.push_back(std::move(handler));
}

void Window::on_cursor_move(CursorMoveHandler handler)
{
	cursor_move_handlers.push_back(std::move(handler));
}

void Window::on_drag(int button, DragHandler handler)
{
	drag_handlers[button].push_back(std::move(handler));
}

Window Window::from(const Session &session, const Device &device)
{
	auto &ldev = device.logical;
	auto &pdev = device.physical;

	Window result;

	glfwInit();
	result.handle = glfwCreateWindow(1024, 1024, "ugp", nullptr, nullptr);

	glfwSetWindowUserPointer(result.handle, &result);
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
		.setPresentMode(vk::PresentModeKHR::eFifo)
		.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
		.setSurface(surface);

	result.swapchain = ldev.createSwapchainKHR(swapchain_info);

	auto swapchain_images = ldev.getSwapchainImagesKHR(result.swapchain);

	result.images.reserve(swapchain_images.size());
	for (auto &handle : swapchain_images) {
		Image image;
		image.device = device;
		image.handle = handle;
		image.layout = vk::ImageLayout::ePresentSrcKHR;
		image.extent = result.extent();
		image.format = result.format;

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

Image &Window::image(size_t index)
{
	return images[index];
}

const Image &Window::image(size_t index) const
{
	return images[index];
}
