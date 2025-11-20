#include "rhi/window.hpp"

bool Window::alive() const
{
	return not glfwWindowShouldClose(handle);
}

void Window::poll() const
{
	glfwPollEvents();
}

vk::Extent2D Window::extent() const
{
	int width;
	int height;

	glfwGetFramebufferSize(handle, &width, &height);

	return vk::Extent2D(width, height);
}

Window::Frame Window::next_frame()
{
	frame_index = (frame_index + 1) % frames_in_flight;
	auto &frame = frames[frame_index];
	frame.extent = extent();
	return frame;
}

Window Window::from(const Session &session, const Device &device)
{
	auto &ldev = device.logical;
	auto &pdev = device.physical;

	Window result;

	glfwInit();
	result.handle = glfwCreateWindow(1024, 1024, "ugp", nullptr, nullptr);

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
