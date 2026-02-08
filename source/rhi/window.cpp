#include <cstdlib>
#include <iostream>
#include <print>
#include <utility>

#include "rhi/vk.hpp"
#include "rhi/window.hpp"

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

VkExtent2D Window::extent() const
{
	int width;
	int height;

	glfwGetFramebufferSize(handle, &width, &height);

	return VkExtent2D {
		static_cast <uint32_t> (width),
		static_cast <uint32_t> (height),
	};
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
	auto ldev = device.logical;
	auto pdev = device.physical;

	Window result;

	result.handle = glfwCreateWindow(options.width, options.height, options.title, nullptr, nullptr);
	result.handler_index = handler_tables.size();
	handler_tables.emplace_back();

	auto user = reinterpret_cast <void *> (result.handler_index);
	glfwSetWindowUserPointer(result.handle, user);
	glfwSetMouseButtonCallback(result.handle, dispatch_mouse_button);
	glfwSetCursorPosCallback(result.handle, dispatch_cursor_pos);

	auto surface = VkSurfaceKHR(VK_NULL_HANDLE);
	auto sresult = glfwCreateWindowSurface(session.handle, result.handle, nullptr, &surface);
	if (sresult != VK_SUCCESS) {
		std::println(std::cerr, "failed to create window surface: {}", static_cast <int> (sresult));
		std::abort();
	}
	result.surface = surface;

	result.format = VK_FORMAT_B8G8R8A8_UNORM;

	VkSurfaceCapabilitiesKHR surface_capabilities {};
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, surface, &surface_capabilities);

	VkSwapchainCreateInfoKHR swapchain_info {};
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.surface = surface;
	swapchain_info.minImageCount = surface_capabilities.minImageCount;
	swapchain_info.imageFormat = result.format;
	swapchain_info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	swapchain_info.imageExtent = result.extent();
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_info.preTransform = surface_capabilities.currentTransform;
	swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_info.presentMode = options.present_mode;
	swapchain_info.clipped = VK_TRUE;
	swapchain_info.oldSwapchain = VK_NULL_HANDLE;

	auto cresult = vkCreateSwapchainKHR(ldev, &swapchain_info, nullptr, &result.swapchain);
	if (cresult != VK_SUCCESS) {
		std::println(std::cerr, "failed to create swapchain: {}", static_cast <int> (cresult));
		std::abort();
	}

	auto swapchain_image_count = uint32_t(0);
	cresult = vkGetSwapchainImagesKHR(ldev, result.swapchain, &swapchain_image_count, nullptr);
	if (cresult != VK_SUCCESS) {
		std::println(std::cerr, "failed to query swapchain image count: {}", static_cast <int> (cresult));
		std::abort();
	}

	auto swapchain_images = std::vector <VkImage> (swapchain_image_count);
	cresult = vkGetSwapchainImagesKHR(ldev, result.swapchain, &swapchain_image_count, swapchain_images.data());
	if (cresult != VK_SUCCESS) {
		std::println(std::cerr, "failed to query swapchain images: {}", static_cast <int> (cresult));
		std::abort();
	}

	result.images.reserve(swapchain_images.size());
	for (auto handle : swapchain_images) {
		Image image;
		image.device = ldev;
		image.handle = handle;
		image.layout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		image.description.extent = VkExtent3D { options.width, options.height, 1 };
		image.description.format = result.format;
		image.description.aspect = VK_IMAGE_ASPECT_COLOR_BIT;

		VkImageSubresourceRange view_range {};
		view_range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		view_range.baseArrayLayer = 0;
		view_range.baseMipLevel = 0;
		view_range.layerCount = 1;
		view_range.levelCount = 1;

		VkImageViewCreateInfo view_info {};
		view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image = handle;
		view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		view_info.subresourceRange = view_range;
		view_info.format = result.format;

		cresult = vkCreateImageView(ldev, &view_info, nullptr, &image.view);
		if (cresult != VK_SUCCESS) {
			std::println(std::cerr, "failed to create swapchain image view: {}", static_cast <int> (cresult));
			std::abort();
		}

		result.images.push_back(image);
	}

	result.frame_index = 0;
	result.frames_in_flight = result.images.size();

	VkFenceCreateInfo fence_info {};
	fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo semaphore_info {};
	semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	result.frames.resize(result.frames_in_flight);
	for (auto &frame : result.frames) {
		frame.swapchain = result.swapchain;
		cresult = vkCreateFence(ldev, &fence_info, nullptr, &frame.fence);
		if (cresult != VK_SUCCESS) {
			std::println(std::cerr, "failed to create frame fence: {}", static_cast <int> (cresult));
			std::abort();
		}

		cresult = vkCreateSemaphore(ldev, &semaphore_info, nullptr, &frame.rendered);
		if (cresult != VK_SUCCESS) {
			std::println(std::cerr, "failed to create render semaphore: {}", static_cast <int> (cresult));
			std::abort();
		}

		cresult = vkCreateSemaphore(ldev, &semaphore_info, nullptr, &frame.presented);
		if (cresult != VK_SUCCESS) {
			std::println(std::cerr, "failed to create present semaphore: {}", static_cast <int> (cresult));
			std::abort();
		}
	}

	return result;
}

} // namespace rcgp
