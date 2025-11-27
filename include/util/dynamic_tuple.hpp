#pragma once

#include <cstddef>
#include <vector>

#include "align.hpp"
#include "trivial_tuple.hpp"

template <typename ... Ts>
class dynamic_tuple;

template <typename T, typename ... Ts>
class dynamic_tuple <T[], Ts...> {
	using statics_t = trivial_tuple <Ts...>;

	static constexpr size_t static_count = sizeof...(Ts);
	
	statics_t statics_storage;
	std::vector <T> dynamics_storage;
public:
	dynamic_tuple() = default;

	explicit dynamic_tuple(size_t elements) : dynamics_storage(elements) {}

	template <size_t Index>
	auto &get() {
		static_assert(Index < static_count, "dynamic_tuple static index out of range");
		return statics_storage.template get <Index> ();
	}

	template <size_t Index>
	const auto &get() const {
		static_assert(Index < static_count, "dynamic_tuple static index out of range");
		return statics_storage.template get <Index> ();
	}

	template <size_t Index>
	static consteval size_t offset() {
		static_assert(Index < static_count, "dynamic_tuple static offset out of range");
		return statics_t::template offset <Index> ();
	}

	static constexpr size_t dynamic_offset() {
		return align_up(sizeof(statics_t), alignof(T));
	}

	static constexpr size_t static_size() {
		return dynamic_offset();
	}

	static constexpr size_t size(size_t elements) {
		return static_size() + sizeof(T) * elements;
	}

	std::vector <T> &dynamic() {
		return dynamics_storage;
	}

	const std::vector <T> &dynamic() const {
		return dynamics_storage;
	}

	const statics_t &statics() const {
		return statics_storage;
	}

	statics_t &statics() {
		return statics_storage;
	}
};
