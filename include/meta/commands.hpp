#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>
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

// Deferred specialization: a queue of pending command operators.
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

// Live specialization: refers to a CommandBuffer handle and records eagerly.
//
// The recording state lives in a shared flag (shared_ptr) so that all chains spawned
// from the same module — copies returned by `operator|` — observe the same recording
// state. The original module and its chain temporaries agree on whether
// `vkBeginCommandBuffer` has been issued, so `finalize()` correctly emits
// `vkEndCommandBuffer` regardless of which copy it's called on.
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

	// End recording (if needed) and return the underlying buffer for submission.
	// Clears the shared recording flag so the next directive will reset+begin again.
	vk::CommandBuffer finalize() {
		if (*recording) {
			handle.end();
			*recording = false;
		}
		return handle;
	}

	// Eagerly emit a labeled scope on the bound buffer.
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

// Deferred + deferred → deferred (current concat behavior, with effect normalization).
template <typename ... A, typename ... B>
auto operator|(const Commands <false, A...> &a, const Commands <false, B...> &b)
{
	using normalized = normalize_effects_t <Tlist <A..., B...>>;
	using result_t = commands_from_t <false, normalized>;
	result_t result;
	result.append_range(a);
	result.append_range(b);
	return result;
}

// nullptr | deferred — kept for back-compat with chains that start with `nullptr |`.
template <typename ... E>
auto operator|(const std::nullptr_t &, const Commands <false, E...> &x)
{
	return x;
}

// Live + deferred → live: flush deferred ops onto the bound buffer immediately and
// propagate the live module forward (with the normalized effect list).
template <typename ... A, typename ... B>
auto operator|(Commands <true, A...> live, const Commands <false, B...> &deferred)
{
	using normalized = normalize_effects_t <Tlist <A..., B...>>;
	using result_t = commands_from_t <true, normalized>;

	live.ensure_recording();
	for (auto &op : deferred)
		op(live.handle, *live.sctx);

	result_t result;
	result.handle = live.handle;
	result.recording = live.recording;
	result.sctx = live.sctx;
	return result;
}

// Live + live is intentionally NOT declared. Concatenating two bound modules has no
// well-defined meaning ("which buffer wins?") so we forbid it at the type level.
TYPE_TRAIT(is_commands);
	template <bool Live, typename ... Effects>
	TYPE_TRAIT_INCLUDES(is_commands, Commands <Live, Effects...>);

} // namespace rcgp
