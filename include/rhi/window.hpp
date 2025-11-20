#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "device.hpp"

struct Window {
	GLFWwindow *handle;
	vk::SurfaceKHR surface;

	vk::Format format;
	vk::SwapchainKHR swapchain;
	std::vector <vk::ImageView> views;

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

	static Window from(const Session &session, const Device &device);
};
