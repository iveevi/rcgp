#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "device.hpp"
#include "image.hpp"

struct Window {
	GLFWwindow *handle;
	vk::SurfaceKHR surface;

	vk::Format format;
	vk::SwapchainKHR swapchain;
	std::vector <Image> images;

	struct Frame {
		vk::Fence fence;
		vk::Semaphore presented;
		vk::Semaphore rendered;
		vk::Extent2D extent;
		uint32_t image_index;
	};

	std::vector <Frame> frames;

	size_t frames_in_flight;
	size_t frame_index;

	bool alive() const;

	void poll() const;

	vk::Extent2D extent() const;

	Frame next_frame();

	Image &image(size_t index);
	const Image &image(size_t index) const;

	static Window from(const Session &session, const Device &device);
};
