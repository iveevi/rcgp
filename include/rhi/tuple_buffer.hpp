#pragma once

#include <cstring>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "buffer.hpp"
#include "../util/dynamic_tuple.hpp"
#include "../util/trivial_tuple.hpp"
#include "../util/logging.hpp"

template <typename T>
struct TupleBuffer {};

template <typename ... Ts>
struct TupleBuffer <trivial_tuple <Ts...>> : Buffer {
	using value_type = trivial_tuple <Ts...>;

	TupleBuffer() = default;

	TupleBuffer(const Buffer &buffer) : Buffer(buffer) {}

	void upload(const value_type &value, vk::DeviceSize relative_offset = 0) const {
		Buffer::upload(&value, sizeof(value), relative_offset);
	}

	static TupleBuffer from(const Device &device, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
		Buffer base = Buffer::from(device, sizeof(value_type), usage, properties);
		return TupleBuffer(base);
	}
};

template <typename T, typename ... Ts>
struct TupleBuffer <dynamic_tuple <T[], Ts...>> : Buffer {
	using value_type = dynamic_tuple <T[], Ts...>;
	using statics_t = trivial_tuple <Ts...>;

	size_t max_elements = 0;

	TupleBuffer() = default;

	TupleBuffer(const Buffer &buffer, size_t max_elements_) : Buffer(buffer), max_elements(max_elements_) {}

	void upload(const value_type &value, size_t elements, vk::DeviceSize relative_offset = 0) const {
		if (elements > max_elements)
			fatal("dynamic tuple upload exceeds allocation ({} > {})", elements, max_elements);

		if (elements > value.dynamics().size())
			fatal("dynamic tuple upload elements exceed source size ({} > {})", elements, value.dynamics().size());

		auto static_bytes = value_type::statics_size();
		auto dynamics_bytes = sizeof(T) * elements;

		std::vector <std::byte> staging(static_bytes + dynamics_bytes);

		const auto &statics = value.statics();
		std::memcpy(staging.data(), &statics, sizeof(statics));

		if (static_bytes > sizeof(statics))
			std::memset(staging.data() + sizeof(statics), 0, static_bytes - sizeof(statics));

		auto dptr = staging.data() + static_bytes;
		std::memcpy(dptr, value.dynamics().data(), dynamics_bytes);

		Buffer::upload(staging.data(), staging.size(), relative_offset);
	}

	static TupleBuffer from(const Device &device, size_t max_elements_, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
		Buffer base = Buffer::from(device, value_type::size(max_elements_), usage, properties);
		return TupleBuffer(base, max_elements_);
	}
};
