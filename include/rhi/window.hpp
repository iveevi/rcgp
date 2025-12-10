#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include "device.hpp"
#include "image.hpp"

enum class Key : int {
	W = GLFW_KEY_W,
	A = GLFW_KEY_A,
	S = GLFW_KEY_S,
	D = GLFW_KEY_D,
	Q = GLFW_KEY_Q,
	E = GLFW_KEY_E,
	Space = GLFW_KEY_SPACE,
	Escape = GLFW_KEY_ESCAPE,
	Up = GLFW_KEY_UP,
	Down = GLFW_KEY_DOWN,
	Left = GLFW_KEY_LEFT,
	Right = GLFW_KEY_RIGHT,
	ShiftLeft = GLFW_KEY_LEFT_SHIFT,
	ShiftRight = GLFW_KEY_RIGHT_SHIFT,
	ControlLeft = GLFW_KEY_LEFT_CONTROL,
	ControlRight = GLFW_KEY_RIGHT_CONTROL,
	AltLeft = GLFW_KEY_LEFT_ALT,
	AltRight = GLFW_KEY_RIGHT_ALT,
	Tab = GLFW_KEY_TAB,
	Enter = GLFW_KEY_ENTER,
	Backspace = GLFW_KEY_BACKSPACE,
};

struct Frame {
	vk::SwapchainKHR swapchain;
	vk::Fence fence;
	vk::Semaphore presented;
	vk::Semaphore rendered;
	vk::Extent2D extent;
	uint32_t image_index;
};

struct Window {
	GLFWwindow *handle;
	vk::SurfaceKHR surface;

	vk::Format format;
	vk::SwapchainKHR swapchain;
	std::vector <Image> images;

	using MouseButtonHandler = std::function <void (int button, int action, int mods)>;
	using CursorMoveHandler = std::function <void (double xpos, double ypos, double dx, double dy)>;
	using DragHandler = std::function <void (double xpos, double ypos, double dx, double dy)>;

	std::vector <Frame> frames;
	std::vector <MouseButtonHandler> mouse_button_handlers;
	std::vector <CursorMoveHandler> cursor_move_handlers;
	std::unordered_map <int, std::vector <DragHandler>> drag_handlers;

	size_t frames_in_flight;
	size_t frame_index;

	// Handler states
	int dragging_button = -1;
	bool cursor_initialized = false;
	double last_x = 0.0;
	double last_y = 0.0;

	void poll() const;
	void close() const;
	
	bool alive() const;
	bool is_pressed(Key key) const;

	vk::Extent2D extent() const;

	Frame next_frame();

	void on_mouse_button(MouseButtonHandler handler);
	void on_cursor_move(CursorMoveHandler handler);
	void on_drag(int button, DragHandler handler);

	Image &image(size_t index);
	const Image &image(size_t index) const;

	static Window from(const Session &session, const Device &device);
};
