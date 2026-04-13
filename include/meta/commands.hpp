#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <vector>

#include "../rhi/command_buffer.hpp"
#include "../rhi/image.hpp"
#include "command_normalization.hpp"

namespace rcgp {

struct TargetRegistration {
	Image *image = nullptr;
	bool is_depth = false;
};

struct SerializationContext {
	size_t pplid;
	std::map <void *, TargetRegistration> target_images;
};

using command_operator = std::function <void (const CommandBuffer &, SerializationContext &)>;

template <bool Live, typename ... Effects>
struct Commands;

template <bool Live, typename List>
struct commands_from;

template <bool Live, typename ... Effects>
struct commands_from <Live, Tlist <Effects...>> {
	using type = Commands <Live, Effects...>;
};

template <bool Live, typename List>
using commands_from_t = typename commands_from <Live, std::remove_cv_t <List>>::type;

template <typename ... Effects>
struct Commands <false, Effects...> : std::vector <command_operator> {
	using std::vector <command_operator> ::vector;

	void serialize(const CommandBuffer &cmd, SerializationContext &sctx) const {
		for (auto &op : *this)
			op(cmd, sctx);
	}

	auto label(std::string name) const {
		Commands result;
		result.push_back([=](const CommandBuffer &cmd, SerializationContext &) {
			cmd.begin_label(name);
		});
		result.append_range(*this);
		result.push_back([](const CommandBuffer &cmd, SerializationContext &) {
			cmd.end_label();
		});
		return result;
	}
};

// shared recording flag so chain copies from operator| agree on begin/end state
template <typename ... Effects>
struct Commands <true, Effects...> {
	CommandBuffer handle;
	std::shared_ptr <bool> recording = std::make_shared <bool> (false);
	std::shared_ptr <SerializationContext> sctx = std::make_shared <SerializationContext> ();

	Commands() = default;
	explicit Commands(const CommandBuffer &cmd) : handle(cmd) {}

	void ensure_recording() {
		if (not *recording) {
			handle.reset();
			handle.begin(vk::CommandBufferBeginInfo());
			*recording = true;
			*sctx = SerializationContext {};
		}
	}

	vk::CommandBuffer finalize() {
		if (*recording) {
			handle.end();
			*recording = false;
		}
		return handle;
	}

	auto &label(std::string name) {
		ensure_recording();
		handle.begin_label(name);
		return *this;
	}

	auto &end_label() {
		handle.end_label();
		return *this;
	}
};

template <typename ... Effects>
using LiveCommands = Commands <true, Effects...>;

template <typename ... Effects>
using DeferredCommands = Commands <false, Effects...>;

// Live + live is ambiguous so it shall not be defined
template <typename ... A, typename ... B>
auto operator|(const Commands <false, A...> &a, const Commands <false, B...> &b)
{
	constexpr auto norm = normalize_effects(Tlist <A..., B...> {});
	if constexpr (not std::is_same_v <std::decay_t <decltype(norm.second)>, int>)
		static_assert(false, norm.second);
	using result_t = commands_from_t <false, decltype(norm.first)>;
	result_t result;
	result.append_range(a);
	result.append_range(b);
	return result;
}

template <typename ... E>
auto operator|(const std::nullptr_t &, const Commands <false, E...> &x)
{
	return x;
}

template <typename ... A, typename ... B>
auto operator|(Commands <true, A...> live, const Commands <false, B...> &deferred)
{
	constexpr auto norm = normalize_effects(Tlist <A..., B...> {});
	if constexpr (not std::is_same_v <std::decay_t <decltype(norm.second)>, int>)
		static_assert(false, norm.second);
	using result_t = commands_from_t <true, decltype(norm.first)>;

	live.ensure_recording();
	for (auto &op : deferred)
		op(live.handle, *live.sctx);

	result_t result;
	result.handle = live.handle;
	result.recording = live.recording;
	result.sctx = live.sctx;
	return result;
}

TYPE_TRAIT(is_commands);
	template <bool Live, typename ... Effects>
	TYPE_TRAIT_INCLUDES(is_commands, Commands <Live, Effects...>);

} // namespace rcgp
