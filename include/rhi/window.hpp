#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

#include <GLFW/glfw3.h>

#include "device.hpp"
#include "image.hpp"
#include "vk.hpp"

namespace rcgp {

enum class Key : int {
	W = GLFW_KEY_W,
	A = GLFW_KEY_A,
	S = GLFW_KEY_S,
	D = GLFW_KEY_D,
	Q = GLFW_KEY_Q,
	E = GLFW_KEY_E,
	R = GLFW_KEY_R,
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
	F12 = GLFW_KEY_F12,
};

enum class MouseButton : int {
	Left = GLFW_MOUSE_BUTTON_LEFT,
	Right = GLFW_MOUSE_BUTTON_RIGHT,
	Middle = GLFW_MOUSE_BUTTON_MIDDLE,
};

struct Frame {
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	VkFence fence = VK_NULL_HANDLE;
	VkSemaphore presented = VK_NULL_HANDLE;
	VkSemaphore rendered = VK_NULL_HANDLE;
	VkExtent2D extent {};
	uint32_t image_index;
};

using MouseButtonHandler = std::function <void (int button, int action, int mods)>;
using CursorMoveHandler = std::function <void (double xpos, double ypos, double dx, double dy)>;
using DragHandler = std::function <void (double xpos, double ypos, double dx, double dy)>;

struct Window {
	GLFWwindow *handle;
	VkSurfaceKHR surface = VK_NULL_HANDLE;

	VkFormat format = VK_FORMAT_UNDEFINED;
	VkSwapchainKHR swapchain = VK_NULL_HANDLE;
	std::vector <Image> images;

	std::vector <Frame> frames;
	size_t frames_in_flight;
	size_t frame_index;
	
	std::intptr_t handler_index;

	void poll() const;
	void close() const;
	
	bool alive() const;
	bool is_pressed(Key key) const;
	void set_input_mode(int mode, int value) const;

	VkExtent2D extent() const;

	float aspect_ratio() const {
		return float(extent().width) / extent().height;
	}

	Frame next_frame();

	void on_mouse_button(MouseButtonHandler handler);
	void on_cursor_move(CursorMoveHandler handler);
	void on_drag(MouseButton button, DragHandler handler);

	struct Options {
		uint32_t width;
		uint32_t height;
		const char *const title;
		VkPresentModeKHR present_mode;
	};

	static Window from(const Session &session, const Device &device, const Options &options);
};

} // namespace rcgp
