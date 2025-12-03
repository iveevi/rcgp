#pragma once

#include "mirror.hpp"
#include "../rhi/buffer.hpp"

template <reflected T, template <typename> typename Engine = layouts::std430>
struct MirrorBuffer : Buffer {};

template <reflected T, template <typename> typename Engine>
requires (is_static_v <T>)
struct MirrorBuffer <T, Engine> : Buffer {
	using value_type = data_mirror <T, Engine> ::type;

	value_type new_value() const {
		return value_type();
	}

	void write(const value_type &data) const
	{
		Buffer::write(&data, sizeof(value_type), 0);
	}

	static MirrorBuffer from(const Device &device, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
	{
		auto base = Buffer::from(device, sizeof(value_type), usage, properties);

		MirrorBuffer result;
		static_cast <Buffer &> (result) = base;
		return result;
	}
};

template <reflected T, template <typename> typename Engine>
requires (is_dynamic_v <T>)
struct MirrorBuffer <T, Engine> : Buffer {
	using value_type = data_mirror <T, Engine> ::type;
	
	value_type new_value() const {
		return value_type();
	}

	void write(const value_type &data) const
	{
		// TODO: do this all in one mapped context
		auto [dyn, offset] = dynamic_part <T> (data);
		if (offset > 0)
			Buffer::write(&data, offset, 0);

		using element = std::decay_t <decltype(dyn)> ::value_type;

		Buffer::write(dyn.data(), dyn.size() * sizeof(element), offset);
	}

	static MirrorBuffer from(const Device &device, size_t max_elements, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties)
	{
		value_type data;
		auto [dyn, offset] = dynamic_part <T> (data);
		using element = std::decay_t <decltype(dyn)> ::value_type;

		auto base = Buffer::from(device, offset + max_elements * sizeof(element), usage, properties);

		MirrorBuffer result;
		static_cast <Buffer &> (result) = base;
		return result;
	}
};
