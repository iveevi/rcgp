#pragma once

#include "../dsl/instructions.hpp"

// Descriptor group allocation
template <auto &rsrc, size_t I>
struct group_allocation_record {};

template <auto &rsrc, size_t I>
bool fill_allocation(group_allocation_map &map, group_allocation_record <rsrc, I>)
{
	map.emplace((void *) &rsrc, I);
	return true;
}

template <typename ... Records>
auto new_allocation(std::tuple <Records...> records)
{
	group_allocation_map map;
	std::apply([&](auto ... xs) {
		std::make_tuple(fill_allocation(map, xs)...);
	}, records);
	return map;
}
