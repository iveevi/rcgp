#pragma once

#include "../rhi/device.hpp"
#include <type_traits>
#include <utility>
#include "reference_introspection.hpp"
#include "resources.hpp"
#include "expand_reflection.hpp"

template <auto &ref, bool resolved = true>
struct DescriptorFor : vk::DescriptorSet {};

// Descriptor writes between descriptor of a resource and its resource mirror
template <typename T>
struct resource_group_value_from_reflection {};

template <typename T>
struct resource_group_value_from_reflection <resource_group_reflection <T>> {
	using type = T;
};

template <typename Original, typename ... Ts>
struct resource_group_value_from_reflection <aggregate_reflection <Original, Ts...>> {
	using type = Original;
};

template <typename R>
struct reflection_unwrap {
	using type = R;
};

template <auto &ref, typename T>
struct reflection_unwrap <reference_reflection <ref, T>> {
	using type = T;
};

template <typename T>
struct bindings_from_reflection : std::integral_constant <size_t, 1> {};

template <typename T>
struct bindings_from_reflection <resource_group_reflection <T>>
	: std::integral_constant <size_t, expand_reflection_t <T> ::field_count> {};

template <typename Original, typename ... Ts>
struct bindings_from_reflection <aggregate_reflection <Original, Ts...>>
	: std::integral_constant <size_t, sizeof...(Ts)> {};

template <auto &ref, bool resolved>
struct DescriptorWritePair {
	const DescriptorFor <ref, resolved> &descriptor;
	const ResourceTypeFor <ref> &resource;

	using ref_base = reference_base_t <ref>;
	using ref_reflection = typename reflection_unwrap <typename ref_base::reflection> ::type;
	static constexpr bool is_group = is_resource_group_reflection <ref_reflection> ::value
		|| is_aggregate_reflection_v <ref_reflection>;
	static constexpr size_t bindings = is_group
		? bindings_from_reflection <ref_reflection> ::value
		: 1;

	struct DescriptorInfoUnion {
		vk::DescriptorImageInfo image;
		vk::DescriptorBufferInfo buffer;
	};

	std::array <DescriptorInfoUnion, bindings> descriptor_infos;

	void bind(const std::span <vk::WriteDescriptorSet, bindings> &writes) {
		if constexpr (is_group) {
			using group_value = typename resource_group_value_from_reflection <ref_reflection> ::type;
			using group_reflection = expand_reflection_t <group_value>;
			static_assert(is_aggregate_reflection_v <group_reflection>,
				"resource group must wrap an aggregate type");

			auto bind_one = [&]<size_t I>() {
				using field_t = typename group_reflection::template field_type <I>;
				auto &field = resource.template get <I> ();

				writes[I]
					.setDstSet(descriptor)
					.setDstBinding(I);

				// TODO: again need a method to go from reflection/resource type to descriptor type
				if constexpr (is_sampler_reflection_v <field_t>) {
					descriptor_infos[I].image = field.descriptor_info();
					writes[I]
						.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
						.setImageInfo(descriptor_infos[I].image);
				} else if constexpr (is_storage_buffer_reflection_v <field_t>) {
					descriptor_infos[I].buffer = field.descriptor_info();
					writes[I]
						.setDescriptorType(vk::DescriptorType::eStorageBuffer)
						.setBufferInfo(descriptor_infos[I].buffer);
				} else {
					descriptor_infos[I].buffer = field.descriptor_info();
					writes[I]
						.setDescriptorType(vk::DescriptorType::eUniformBuffer)
						.setBufferInfo(descriptor_infos[I].buffer);
				}
			};

			[&]<size_t ... Is>(std::index_sequence <Is...>) {
				(bind_one.template operator() <Is> (), ...);
			} (std::make_index_sequence <bindings> ());
		} else {
			static constexpr bool is_sampler = is_sampler_v <ref_base>;
			if constexpr (is_sampler) {
				descriptor_infos[0].image = resource.descriptor_info();
				writes[0]
					.setDstSet(descriptor)
					.setDstBinding(0)
					.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
					.setImageInfo(descriptor_infos[0].image);
			} else if constexpr (is_storage_buffer_reflection_v <ref_reflection>) {
				descriptor_infos[0].buffer = resource.descriptor_info();
				writes[0]
					.setDstSet(descriptor)
					.setDstBinding(0)
					.setDescriptorType(vk::DescriptorType::eStorageBuffer)
					.setBufferInfo(descriptor_infos[0].buffer);
			} else {
				descriptor_infos[0].buffer = resource.descriptor_info();
				writes[0]
					.setDstSet(descriptor)
					.setDstBinding(0)
					.setDescriptorType(vk::DescriptorType::eUniformBuffer)
					.setBufferInfo(descriptor_infos[0].buffer);
			}
		}
	}
};

template <auto &ref, bool resolved>
DescriptorWritePair(const DescriptorFor <ref, resolved> &,
	const ResourceTypeFor <ref> &)
	-> DescriptorWritePair <ref, resolved>;

template <auto &...refs, bool ... rs>
[[nodiscard]] auto Device::update_descriptors(DescriptorWritePair <refs, rs> ... dwpairs)
{
	static_assert(sizeof...(dwpairs) > 0);

	static constexpr size_t writes_count = (decltype(dwpairs)::bindings + ...);

	std::array <vk::WriteDescriptorSet, writes_count> writes;

	auto progress = 0;
	auto bind = [&](auto &dwpair) {
		using pair_t = std::remove_reference_t <decltype(dwpair)>;
		constexpr size_t size = pair_t::bindings;
		auto span = std::span <vk::WriteDescriptorSet, size> {
			&writes[progress], size
		};
		dwpair.bind(span);
		progress += size;
	};

	(bind(dwpairs), ...);

	logical.updateDescriptorSets(writes, nullptr);

	if constexpr (sizeof...(dwpairs) == 1) {
		return DescriptorFor <refs...[0], true>
			((dwpairs...[0]).descriptor);
	} else {
		return std::make_tuple(DescriptorFor <refs, true> (dwpairs.descriptor)...);
	}
}
