#pragma once

#include <array>
#include <cstdlib>

#include "../dsl/instruction_block.hpp"
#include "../rhi/device.hpp"
#include "../util/align.hpp"
#include "../util/cti.hpp"
#include "witnesses.hpp"
#include "resources.hpp"
#include "resources_collect.hpp"
#include "shader_stage_conversion.hpp"

namespace rcgp {

template <auto &ref, typename ... Wrappers>
consteval size_t push_constant_offset_for(Tlist <Wrappers...>)
{
	size_t offset = 0;
	bool passed = false;
	auto accum = [&] <typename W> () {
		using R = W::type;
		using T = ResourceTypeFor <W::handle>;

		if constexpr (std::same_as <typename W::contract, contract <ref>>)
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
	using Reference = reference_base_of <ref>;

	auto stage_flags = VkShaderStageFlags((stage_to_flag(Ss) | ...));
	if constexpr (is_resource_group_v <Reference>) {
		using T = Reference::struct_type;

		constexpr size_t bindings = T::field_count;
		std::array <VkDescriptorSetLayoutBinding, bindings> dslbs {};

		auto fill_one = [&] <size_t I> () {
			using Resource = T::fields::template get <I>;

			VkDescriptorType dtype = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			if constexpr (is_sampler_v <Resource>) {
				dtype = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			} else if constexpr (is_storage_buffer_v <Resource>) {
				dtype = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			}

			dslbs[I] = VkDescriptorSetLayoutBinding {
				.binding = uint32_t(I),
				.descriptorType = dtype,
				.descriptorCount = 1,
				.stageFlags = stage_flags,
			};
		};

		constexpr_for(Is, bindings,
			(fill_one.template operator() <Is> (), ...)
		);

		VkDescriptorSetLayoutCreateInfo dsl_info {};
		dsl_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		dsl_info.bindingCount = dslbs.size();
		dsl_info.pBindings = dslbs.data();

		auto dsl = VkDescriptorSetLayout(VK_NULL_HANDLE);
		auto result = vkCreateDescriptorSetLayout(device.logical, &dsl_info, nullptr, &dsl);
		if (result != VK_SUCCESS)
			std::abort();
		return dsl;
	} else {
		VkDescriptorType dtype = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		if constexpr (is_sampler_v <Reference>)
			dtype = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		else if constexpr (is_storage_buffer_v <Reference>)
			dtype = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		auto dslbs = std::array {
			VkDescriptorSetLayoutBinding {
				.binding = 0,
				.descriptorType = dtype,
				.descriptorCount = 1,
				.stageFlags = stage_flags,
			}
		};

		VkDescriptorSetLayoutCreateInfo dsl_info {};
		dsl_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		dsl_info.bindingCount = dslbs.size();
		dsl_info.pBindings = dslbs.data();

		auto dsl = VkDescriptorSetLayout(VK_NULL_HANDLE);
		auto result = vkCreateDescriptorSetLayout(device.logical, &dsl_info, nullptr, &dsl);
		if (result != VK_SUCCESS)
			std::abort();
		return dsl;
	}
}

template <typename ... Ts>
auto wrappers_to_dsls(const Device &device, const Tlist <Ts...> &)
{
	if constexpr (sizeof...(Ts) == 0)
		return std::array <VkDescriptorSetLayout, 0> ();
	else
		return std::array { one_wrapper_to_dsl(device, Ts())... };
}

template <typename ... Wrappers>
auto wrappers_to_pcs(const Tlist <Wrappers...> &)
{
	push_constant_allocation_map map;
	if constexpr (sizeof...(Wrappers) == 0) {
		std::array <VkPushConstantRange, 0> ranges;
		return std::tuple { ranges, map };
	} else {
		std::array <VkPushConstantRange, sizeof...(Wrappers)> ranges {};

		uint32_t offset = 0;
		uint32_t index = 0;

		auto populate = [&] <typename W> () {
			using T = ResourceTypeFor <W::handle>;

			offset = align_up(static_cast <size_t> (offset), alignof(T));
			
			ranges[index++] = VkPushConstantRange {
				.stageFlags = W::flags,
				.offset = offset,
				.size = sizeof(T),
			};

			map.emplace(W::contract::address, offset);

			offset += sizeof(T);
		};

		(populate.template operator() <Wrappers> (), ...);

		return std::tuple { ranges, map };
	}
}

} // namespace rcgp
