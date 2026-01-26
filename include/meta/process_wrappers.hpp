#pragma once

#include <array>

#include "../dsl/instruction_block.hpp"
#include "../rhi/device.hpp"
#include "../util/align.hpp"
#include "../util/cti.hpp"
#include "reference_introspection.hpp"
#include "resources.hpp"
#include "resources_collect.hpp"

template <auto &ref, typename ... Wrappers>
consteval size_t push_constant_offset_for(Tlist <Wrappers...>)
{
	size_t offset = 0;
	bool passed = false;
	auto accum = [&] <typename W> () {
		using R = W::type;
		using T = ResourceTypeFor <W::handle>;

		if constexpr (std::same_as <typename W::reference, reference <ref>>)
			passed = true;
		if (not passed && is_push_constant_v <R>) {
			offset = align_up(offset, alignof(T));
			offset += sizeof(T);
		}
	};
	(accum.template operator() <Wrappers> (), ...);
	return offset;
}

template <auto &ref, ShaderStage ... Ss>
auto one_wrapper_to_dsl(const Device &device, const stage_wrapper <ref, Ss...> &)
{
	using Reference = reference_base_t <ref>;

	auto stage_flags = (stage_to_flag(Ss) | ...);
	if constexpr (is_resource_group_v <Reference>) {
		using Structure = Reference::value_type;
		static_assert(aggregate <Structure>);

		constexpr size_t bindings = Structure::reflection::field_count;
		std::array <vk::DescriptorSetLayoutBinding, bindings> dslbs {};

		auto fill_one = [&] <size_t I> () {
			using Resource = Structure::reflection::template field_type <I>;

			vk::DescriptorType dtype = vk::DescriptorType::eUniformBuffer;
			if constexpr (is_sampler_v <Resource>) {
				dtype = vk::DescriptorType::eCombinedImageSampler;
			} else if constexpr (is_storage_buffer_v <Resource>) {
				dtype = vk::DescriptorType::eStorageBuffer;
			}

			dslbs[I] = vk::DescriptorSetLayoutBinding()
				.setBinding(I)
				.setDescriptorCount(1)
				.setDescriptorType(dtype)
				.setStageFlags(stage_flags);
		};

		constexpr_for(Is, bindings,
			(fill_one.template operator() <Is> (), ...)
		);

		auto dsl_info = vk::DescriptorSetLayoutCreateInfo()
			.setBindings(dslbs);

		return device.logical.createDescriptorSetLayout(dsl_info);
	} else {
		vk::DescriptorType dtype = vk::DescriptorType::eUniformBuffer;
		if constexpr (is_sampler_v <Reference>) {
			dtype = vk::DescriptorType::eCombinedImageSampler;
		} else if constexpr (is_storage_buffer_v <Reference>) {
			dtype = vk::DescriptorType::eStorageBuffer;
		}

		auto dslbs = std::array {
			vk::DescriptorSetLayoutBinding()
				.setBinding(0)
				.setDescriptorCount(1)
				.setDescriptorType(dtype)
				.setStageFlags(stage_flags)
		};

		auto dsl_info = vk::DescriptorSetLayoutCreateInfo()
			.setBindings(dslbs);

		return device.logical.createDescriptorSetLayout(dsl_info);
	}
}

template <typename ... Ts>
auto wrappers_to_dsls(const Device &device, const Tlist <Ts...> &)
{
	if constexpr (sizeof...(Ts) == 0)
		return std::array <vk::DescriptorSetLayout, 0> ();
	else
		return std::array { one_wrapper_to_dsl(device, Ts())... };
}

template <typename ... Wrappers>
auto wrappers_to_pcs(const Tlist <Wrappers...> &)
{
	push_constant_allocation_map map;
	if constexpr (sizeof...(Wrappers) == 0) {
		std::array <vk::PushConstantRange, 0> ranges;
		return std::tuple { ranges, map };
	} else {
		std::array <vk::PushConstantRange, sizeof...(Wrappers)> ranges {};

		uint32_t offset = 0;
		uint32_t index = 0;

		auto populate = [&] <typename W> () {
			using T = ResourceTypeFor <W::handle>;

			offset = align_up(static_cast <size_t> (offset), alignof(T));
			
			ranges[index] = vk::PushConstantRange()
				.setOffset(offset)
				.setSize(sizeof(T))
				.setStageFlags(W::flags);

			map.emplace(W::reference::address, PushConstantAllocation { index, offset });

			offset += sizeof(T);
			index++;
		};

		(populate.template operator() <Wrappers> (), ...);

		return std::tuple { ranges, map };
	}
}
