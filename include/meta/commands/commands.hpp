#pragma once

#include <cstddef>
#include <functional>
#include <vector>

#include "../../rhi/command_buffer.hpp"
#include "../../util/cti.hpp"
#include "../../util/timer.hpp"

struct SerializationContext {
	size_t pplid;
};

// TODO: more efficient std function representation?
// TODO: need to compare with slang version...
// ideally finish up specular lighting...
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
		// TODO: normalize <...>
		auto cmd = Commands <Effects..., Es...> {};
		cmd.append_range(a);
		cmd.append_range(b);
		return cmd;
	}

	friend auto operator|(const std::nullptr_t &, const Commands &x) {
		return x;
	}

	// TODO: allow |= if the result is the same
};

TYPE_TRAIT(is_commands);
	template <typename ... Effects>
	TYPE_TRAIT_INCLUDES(is_commands, Commands <Effects...>);
