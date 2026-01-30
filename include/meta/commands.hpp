#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "../rhi/command_buffer.hpp"
#include "../util/timer.hpp"
#include "command_normalization.hpp"

namespace rcgp {

struct SerializationContext {
	size_t pplid;
};

using command_operator = std::function <void (const CommandBuffer &, SerializationContext &)>;

template <typename ... Effects>
struct Commands : std::vector <command_operator> {
	using std::vector <command_operator> ::vector;

	void serialize(const CommandBuffer &cmd, SerializationContext &sctx) const {
		for (auto &op : *this)
			op(cmd, sctx);
	}

	// TODO: must be in a satisfied state...
	auto &operator()(CommandBuffer &cmd) const {
		TSCOPE("commands serialization");
		TNOTE("{} commands to trace", size());

		SerializationContext aux;

		cmd.reset();
		cmd.begin(vk::CommandBufferBeginInfo());

		serialize(cmd, aux);

		cmd.end();

		return cmd;
	}

	template <typename ... Es>
	friend auto operator|(const Commands &a, const Commands <Es...> &b) {
		using combined = Tlist <Effects..., Es...>;
		using normalized = detail::normalize_effects_t <combined>;
		using cmds = typename normalized::template invoke <Commands>;
		cmds result;
		result.append_range(a);
		result.append_range(b);
		return result;
	}

	friend auto operator|(const std::nullptr_t &, const Commands &x) {
		return x;
	}

	// TODO: allow |= if the result is the same
};

TYPE_TRAIT(is_commands);
	template <typename ... Effects>
	TYPE_TRAIT_INCLUDES(is_commands, Commands <Effects...>);

} // namespace rcgp
