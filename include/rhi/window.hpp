#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string_view>
#include <utility>
#include <vector>

#include <vulkan/vulkan.hpp>

#include "image.hpp"

struct GLFWwindow;

namespace rcgp {

struct Device;
struct Queue;

// Keep GLFW-compatible values without exposing GLFW in the public header.
enum class Key : int {
	Space = 32,
	A = 65,
	D = 68,
	E = 69,
	Q = 81,
	R = 82,
	S = 83,
	W = 87,
	Escape = 256,
	Enter = 257,
	Tab = 258,
	Backspace = 259,
	Delete = 261,
	Right = 262,
	Left = 263,
	Down = 264,
	Up = 265,
	F2 = 291,
	F12 = 301,
	ShiftLeft = 340,
	ControlLeft = 341,
	AltLeft = 342,
	ShiftRight = 344,
	ControlRight = 345,
	AltRight = 346,
};

enum class MouseButton : int {
	Left = 0,
	Right = 1,
	Middle = 2,
};

enum class InputMode : int {
	RawMouseMotion = 0x00033005,
};

enum class CursorMode : int {
	Normal = 0x00034001,
	Hidden = 0x00034002,
	Disabled = 0x00034003,
};

struct Session;
struct Device;

using MouseButtonHandler = std::function <void (int button, int action, int mods)>;
using CursorMoveHandler = std::function <void (double xpos, double ypos, double dx, double dy)>;
using DragHandler = std::function <void (double xpos, double ypos, double dx, double dy)>;
using ScrollHandler = std::function <void (double xoffset, double yoffset)>;

class Window {
	bool is_swapchain_out_of_date = false;
	std::intptr_t handler_index = 0;
public:
	GLFWwindow *handle = nullptr;
	vk::SurfaceKHR surface = nullptr;

	vk::Format format = vk::Format::eUndefined;
	vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo;
	vk::Extent2D swapchain_extent {};
	vk::SwapchainKHR swapchain = nullptr;
	std::vector <ColorTargetImage> images;

	void poll() const;
	void close() const;

	bool alive() const;
	bool is_down(Key key) const;
	bool is_pressed(Key key) const;
	bool was_pressed(Key key) const;
	bool was_released(Key key) const;
	bool is_down(MouseButton button) const;
	bool was_pressed(MouseButton button) const;
	bool was_released(MouseButton button) const;
	std::pair <double, double> cursor_position() const;
	std::pair <double, double> cursor_delta() const;
	std::pair <double, double> scroll_delta() const;
	std::u32string_view character_stream() const;
	void set_cursor_mode(CursorMode mode) const;
	void set_input_mode(InputMode mode, bool value) const;

	vk::Extent2D extent() const;
	bool is_out_of_date();
	float aspect_ratio() const;

	void on_mouse_button(MouseButtonHandler handler);
	void on_cursor_move(CursorMoveHandler handler);
	void on_drag(MouseButton button, DragHandler handler);
	void on_scroll(ScrollHandler handler);

	struct Options {
		uint32_t width = 1024;
		uint32_t height = 1024;
		const char *const title = "RCGP";
		vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo;
	};

	static Window from(const Session &session, const Device &device, const Options &options);

	friend class Device;
	friend class Queue;
};

} // namespace rcgp
