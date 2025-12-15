#pragma once

#include <cstdlib>

#include <vulkan/vulkan.hpp>

#include "../util/sequence.hpp"
#include "static_string.hpp"
#include "field_name_injection.hpp"
#include "macro_hell.hpp"
#include "this_injection.hpp"

// Reflection types
template <typename T>
struct primitive_reflection {};

template <typename Original, typename ... Ts>
struct aggregate_reflection {
	static constexpr size_t field_count = sizeof...(Ts);

	template <size_t I>
	using field_type = Ts...[I];

	// TODO: static_assert check here to ensure at most
	// one dynamic component (and finding the conflicting
	// ones)
};

template <typename T, int64_t N>
struct array_reflection {};

template <auto &ref, typename T>
struct reference_reflection {};

template <typename R, typename ... Args>
struct function_reflection {};

template <typename T>
struct resource_group_reflection {};

template <typename T>
struct push_constant_reflection {};

template <typename T, template <typename> typename L>
struct uniform_buffer_reflection {};

template <typename T, template <typename> typename L>
struct storage_buffer_reflection {};

template <typename T, size_t D>
struct sampler_reflection {};

template <typename T, vk::VertexInputRate>
struct attribute_stream_reflection {};

// Specific kinds of reflection
template <typename T>
struct is_primitive_reflection : std::false_type {};

template <typename U>
struct is_primitive_reflection <primitive_reflection <U>> : std::true_type {};

template <typename T>
constexpr bool is_primitive_reflection_v = is_primitive_reflection <T> ::value;

template <typename T>
struct is_aggregate_reflection : std::false_type {};

template <typename Original, typename ... Ts>
struct is_aggregate_reflection <aggregate_reflection <Original, Ts...>> : std::true_type {};

template <typename T>
constexpr bool is_aggregate_reflection_v = is_aggregate_reflection <T> ::value;

template <typename T>
struct is_dynamic_reflection : std::false_type {};

template <typename T>
struct is_dynamic_reflection <array_reflection <T, -1>> : std::true_type {};

template <typename Original, typename ... Ts>
struct is_dynamic_reflection <aggregate_reflection <Original, Ts...>>
	: std::bool_constant <(is_dynamic_reflection <Ts> ::value || ...)> {};

template <typename T>
constexpr bool is_dynamic_reflection_v = is_dynamic_reflection <T> ::value;

template <typename T>
constexpr bool is_static_reflection_v = !is_dynamic_reflection <T> ::value;

// Querying reflection status
template <typename T>
constexpr bool has_reflection()
{
	if constexpr (requires { T::_ugp_has_reflection; })
		return T::_ugp_has_reflection;
	else
		return false;
}

template <typename T>
constexpr bool has_aggregate_reflection()
{
	if constexpr (has_reflection <T> ())
		return is_aggregate_reflection_v <typename T::reflection>;
	else
		return false;
}

template <typename T>
constexpr bool has_primitive_reflection()
{
	if constexpr (has_reflection <T> ())
		return is_primitive_reflection_v <typename T::reflection>;
	else
		return false;
}

template <typename T>
concept reflected = has_reflection <T> ();

template <typename T>
concept aggregate = has_aggregate_reflection <T> ();

template <typename T>
concept primitive = has_primitive_reflection <T> ();
