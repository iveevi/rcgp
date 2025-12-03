#pragma once

#include <cstddef>

struct InjectionState {
	size_t argidx;
	size_t threadidx;

	InjectionState next(bool argument, bool thread_input) const {
		return {
			.argidx = argidx + argument,
			.threadidx = threadidx + thread_input,
		};
	}
};
