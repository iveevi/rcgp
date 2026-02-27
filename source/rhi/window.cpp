#include "rhi/window.hpp"
#include "rhi/device.hpp"

#include <GLFW/glfw3.h>

#include <array>
#include <unordered_map>
#include <utility>

namespace rcgp {

constexpr size_t key_count = GLFW_KEY_LAST + 1;
constexpr size_t mouse_button_count = GLFW_MOUSE_BUTTON_LAST + 1;

struct HandlerTable {
	int dragging_button = -1;
	bool cursor_initialized = false;
	bool framebuffer_resized = false;
	double last_x = 0.0;
	double last_y = 0.0;
	double frame_cursor_dx = 0.0;
	double frame_cursor_dy = 0.0;
	double frame_scroll_dx = 0.0;
	double frame_scroll_dy = 0.0;
	std::u32string character_stream;

	std::array <bool, key_count> key_down {};
	std::array <bool, key_count> key_pressed {};
	std::array <bool, key_count> key_released {};
	std::array <bool, mouse_button_count> mouse_down {};
	std::array <bool, mouse_button_count> mouse_pressed {};
	std::array <bool, mouse_button_count> mouse_released {};

	std::vector <MouseButtonHandler> mouse_button;
	std::vector <CursorMoveHandler> cursor_move;
	std::vector <ScrollHandler> scroll;
	std::unordered_map <int, std::vector <DragHandler>> drag;
};

inline std::vector <HandlerTable> handler_tables;

bool in_bounds(int value, size_t count)
{
	return value >= 0 && size_t(value) < count;
}

void begin_input_frame(HandlerTable &ht)
{
	ht.key_pressed.fill(false);
	ht.key_released.fill(false);
	ht.mouse_pressed.fill(false);
	ht.mouse_released.fill(false);
	ht.frame_cursor_dx = 0.0;
	ht.frame_cursor_dy = 0.0;
	ht.frame_scroll_dx = 0.0;
	ht.frame_scroll_dy = 0.0;
	ht.character_stream.clear();
}

void clear_down_state(HandlerTable &ht)
{
	ht.dragging_button = -1;
	ht.key_down.fill(false);
	ht.key_pressed.fill(false);
	ht.key_released.fill(false);
	ht.mouse_down.fill(false);
	ht.mouse_pressed.fill(false);
	ht.mouse_released.fill(false);
	ht.character_stream.clear();
}

void dispatch_mouse_button(GLFWwindow *w, int button, int action, int mods)
{
	auto user = glfwGetWindowUserPointer(w);
	auto handler_index = reinterpret_cast <std::intptr_t> (user);

	auto &ht = handler_tables[handler_index];

	if (in_bounds(button, mouse_button_count)) {
		bool down = (action != GLFW_RELEASE);
		bool was_down = ht.mouse_down[size_t(button)];
		if (down && !was_down)
			ht.mouse_pressed[size_t(button)] = true;
		if (!down && was_down)
			ht.mouse_released[size_t(button)] = true;
		ht.mouse_down[size_t(button)] = down;
	}

	for (auto &cb : ht.mouse_button)
		cb(button, action, mods);

	if (action == GLFW_PRESS) {
		ht.dragging_button = button;
		glfwGetCursorPos(w, &ht.last_x, &ht.last_y);
		int framebuffer_width = 0;
		int framebuffer_height = 0;
		int window_width = 0;
		int window_height = 0;
		glfwGetFramebufferSize(w, &framebuffer_width, &framebuffer_height);
		glfwGetWindowSize(w, &window_width, &window_height);

		if (window_width > 0 && window_height > 0) {
			auto sx = double(framebuffer_width) / double(window_width);
			auto sy = double(framebuffer_height) / double(window_height);
			ht.last_x *= sx;
			ht.last_y *= sy;
		}
		ht.cursor_initialized = true;
	} else if (action == GLFW_RELEASE) {
		if (ht.dragging_button == button)
			ht.dragging_button = -1;
	}
}

void dispatch_key(GLFWwindow *w, int key, int, int action, int)
{
	auto user = glfwGetWindowUserPointer(w);
	auto handler_index = reinterpret_cast <std::intptr_t> (user);

	auto &ht = handler_tables[handler_index];
	if (!in_bounds(key, key_count))
		return;
	if (action == GLFW_REPEAT)
		return;

	bool down = (action == GLFW_PRESS);
	bool was_down = ht.key_down[size_t(key)];
	if (down && !was_down)
		ht.key_pressed[size_t(key)] = true;
	if (!down && was_down)
		ht.key_released[size_t(key)] = true;
	ht.key_down[size_t(key)] = down;
}

void dispatch_cursor_pos(GLFWwindow *w, double xpos, double ypos)
{
	auto user = glfwGetWindowUserPointer(w);
	auto handler_index = reinterpret_cast <std::intptr_t> (user);

	auto &ht = handler_tables[handler_index];

	double fx = xpos;
	double fy = ypos;

	int framebuffer_width = 0;
	int framebuffer_height = 0;
	int window_width = 0;
	int window_height = 0;

	glfwGetFramebufferSize(w, &framebuffer_width, &framebuffer_height);
	glfwGetWindowSize(w, &window_width, &window_height);

	if (window_width > 0 && window_height > 0) {
		auto sx = double(framebuffer_width) / double(window_width);
		auto sy = double(framebuffer_height) / double(window_height);
		fx *= sx;
		fy *= sy;
	}

	double dx = 0.0;
	double dy = 0.0;
	if (ht.cursor_initialized) {
		dx = fx - ht.last_x;
		dy = fy - ht.last_y;
	}

	ht.last_x = fx;
	ht.last_y = fy;
	ht.cursor_initialized = true;
	ht.frame_cursor_dx += dx;
	ht.frame_cursor_dy += dy;
	for (auto &cb : ht.cursor_move)
		cb(fx, fy, dx, dy);

	if (ht.dragging_button != -1) {
		auto it = ht.drag.find(ht.dragging_button);
		if (it != ht.drag.end()) {
			for (auto &cb : it->second)
				cb(fx, fy, dx, dy);
		}
	}
}

void dispatch_char(GLFWwindow *w, unsigned int codepoint)
{
	auto user = glfwGetWindowUserPointer(w);
	auto handler_index = reinterpret_cast <std::intptr_t> (user);
	auto &ht = handler_tables[handler_index];
	ht.character_stream.push_back(char32_t(codepoint));
}

void dispatch_framebuffer_size(GLFWwindow *w, int, int)
{
	auto user = glfwGetWindowUserPointer(w);
	auto handler_index = reinterpret_cast <std::intptr_t> (user);
	handler_tables[handler_index].framebuffer_resized = true;
}

void dispatch_scroll(GLFWwindow *w, double xoffset, double yoffset)
{
	auto user = glfwGetWindowUserPointer(w);
	auto handler_index = reinterpret_cast <std::intptr_t> (user);

	auto &ht = handler_tables[handler_index];
	ht.frame_scroll_dx += xoffset;
	ht.frame_scroll_dy += yoffset;
	for (auto &cb : ht.scroll)
		cb(xoffset, yoffset);
}

void dispatch_focus(GLFWwindow *w, int focused)
{
	auto user = glfwGetWindowUserPointer(w);
	auto handler_index = reinterpret_cast <std::intptr_t> (user);
	auto &ht = handler_tables[handler_index];
	if (focused == 0)
		clear_down_state(ht);
}

void Window::poll() const
{
	for (auto &table : handler_tables)
		begin_input_frame(table);
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

bool Window::is_down(Key key) const
{
	int idx = std::to_underlying(key);
	if (!in_bounds(idx, key_count))
		return false;
	return handler_tables[handler_index].key_down[size_t(idx)];
}

bool Window::is_pressed(Key key) const
{
	return is_down(key);
}

bool Window::was_pressed(Key key) const
{
	int idx = std::to_underlying(key);
	if (!in_bounds(idx, key_count))
		return false;
	return handler_tables[handler_index].key_pressed[size_t(idx)];
}

bool Window::was_released(Key key) const
{
	int idx = std::to_underlying(key);
	if (!in_bounds(idx, key_count))
		return false;
	return handler_tables[handler_index].key_released[size_t(idx)];
}

bool Window::is_down(MouseButton button) const
{
	int idx = std::to_underlying(button);
	if (!in_bounds(idx, mouse_button_count))
		return false;
	return handler_tables[handler_index].mouse_down[size_t(idx)];
}

bool Window::was_pressed(MouseButton button) const
{
	int idx = std::to_underlying(button);
	if (!in_bounds(idx, mouse_button_count))
		return false;
	return handler_tables[handler_index].mouse_pressed[size_t(idx)];
}

bool Window::was_released(MouseButton button) const
{
	int idx = std::to_underlying(button);
	if (!in_bounds(idx, mouse_button_count))
		return false;
	return handler_tables[handler_index].mouse_released[size_t(idx)];
}

std::pair <double, double> Window::cursor_position() const
{
	const auto &ht = handler_tables[handler_index];
	return { ht.last_x, ht.last_y };
}

std::pair <double, double> Window::cursor_delta() const
{
	const auto &ht = handler_tables[handler_index];
	return { ht.frame_cursor_dx, ht.frame_cursor_dy };
}

std::pair <double, double> Window::scroll_delta() const
{
	const auto &ht = handler_tables[handler_index];
	return { ht.frame_scroll_dx, ht.frame_scroll_dy };
}

std::u32string_view Window::character_stream() const
{
	return handler_tables[handler_index].character_stream;
}

void Window::set_cursor_mode(CursorMode mode) const
{
	glfwSetInputMode(handle, GLFW_CURSOR, std::to_underlying(mode));
}

void Window::set_input_mode(InputMode mode, bool value) const
{
	glfwSetInputMode(handle, std::to_underlying(mode), value ? GLFW_TRUE : GLFW_FALSE);
}

vk::Extent2D Window::extent() const
{
	return swapchain_extent;
}

float Window::aspect_ratio() const
{
	auto e = extent();
	return e.height > 0 ? (float(e.width) / float(e.height)) : 1.0f;
}

bool Window::needs_swapchain_rebuild()
{
	if (handler_tables[handler_index].framebuffer_resized) {
		swapchain_rebuild_requested = true;
		handler_tables[handler_index].framebuffer_resized = false;
	}

	return swapchain_rebuild_requested;
}

Frame Window::next_frame()
{
	frame_index = (frame_index + 1) % frames_in_flight;
	auto &frame = frames[frame_index];
	frame.extent = swapchain_extent;
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

void Window::on_scroll(ScrollHandler handler)
{
	auto &ht = handler_tables[handler_index];
	ht.scroll.push_back(std::move(handler));
}

Window Window::from(const Session &session, const Device &device, const Options &options)
{
	Window result;

	result.handle = glfwCreateWindow(options.width, options.height, options.title, nullptr, nullptr);
	result.handler_index = handler_tables.size();
	handler_tables.emplace_back();

	auto user = reinterpret_cast <void *> (result.handler_index);
	glfwSetWindowUserPointer(result.handle, user);
	glfwSetMouseButtonCallback(result.handle, dispatch_mouse_button);
	glfwSetKeyCallback(result.handle, dispatch_key);
	glfwSetCursorPosCallback(result.handle, dispatch_cursor_pos);
	glfwSetCharCallback(result.handle, dispatch_char);
	glfwSetFramebufferSizeCallback(result.handle, dispatch_framebuffer_size);
	glfwSetScrollCallback(result.handle, dispatch_scroll);
	glfwSetWindowFocusCallback(result.handle, dispatch_focus);

	VkSurfaceKHR surface;
	glfwCreateWindowSurface(session.handle, result.handle, nullptr, &surface);
	result.surface = surface;

	result.format = vk::Format::eB8G8R8A8Unorm;
	result.present_mode = options.present_mode;
	result.swapchain = nullptr;
	result.frames_in_flight = 0;
	result.frame_index = 0;
	result.swapchain_rebuild_requested = true;

	handler_tables[result.handler_index].framebuffer_resized = true;
	while (not device.rebuild_swapchain(result))
		glfwWaitEvents();

	return result;
}
} // namespace rcgp
