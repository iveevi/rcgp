#pragma once

template <auto &ref, size_t I>
struct group_allocation_record {
	static constexpr void *vptr = &ref;
	static constexpr size_t index = I;
};
