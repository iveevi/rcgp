#pragma once

#include "../rhi/device.hpp"
#include "contract_configuration.hpp"
#include "resources.hpp"
#include "resources_collect.hpp"
#include "shader_stage_conversion.hpp"
#include "intrinsics_raytracing.hpp"

namespace rcgp {

template <typename R>
constexpr vk::DescriptorType resource_descriptor_type_v = []
{
	if constexpr (is_sampler_v <R>)
		return vk::DescriptorType::eCombinedImageSampler;
	else if constexpr (is_storage_buffer_v <R>)
	      return vk::DescriptorType::eStorageBuffer;
	else if constexpr (is_storage_image_v <R>)
	      return vk::DescriptorType::eStorageImage;
	else if constexpr (is_uniform_buffer_v <R>)
	      return vk::DescriptorType::eUniformBuffer;
	else if constexpr (std::is_same_v <R, RaytracingAccelerationStructure>)
		return vk::DescriptorType::eAccelerationStructureKHR;
	else if constexpr (is_resource_array_v <R>)
		return resource_descriptor_type_v <typename R::base>;
} ();

template <typename R>
contract_desc resource_packet(const contract_desc &override)
{
	contract_desc result;
	result.type = resource_descriptor_type_v <R>;
	result.flags = override.flags;

	if constexpr (is_resource_array_v <R>) {
		if constexpr (R::elements >= 0) {
			result.count = R::elements;
		} else {
			result.count = override.count;
			result.flags |= vk::DescriptorBindingFlagBits::eVariableDescriptorCount;
		}
	}

	return result;
};

template <auto &ref, ShaderStage ... Ss>
auto dslbs_for_resource_group(const stage_wrapper <ref, Ss...> &)
{
	// TODO: maybe add template params to resource group to enable partially bound?
	using R = reference_base_of <ref>;
	using Fields = R::struct_type::fields;

	constexpr auto stage_flags = (stage_to_flag(Ss) | ...);

	auto override = contract_desc::overrides[&ref];

	contract_desc packet;
	return constexpr_for(Is, Fields::size,
		return std::tuple {
			std::array {(
				packet = resource_packet
					<typename Fields::template get <Is>> (override),
				vk::DescriptorSetLayoutBinding()
					.setBinding(Is)
					.setDescriptorCount(packet.count)
					.setDescriptorType(packet.type)
					.setStageFlags(stage_flags)
			)...},
			std::array {
				resource_packet
					<typename Fields::template get <Is>> (override)
					.flags...
			},
		};
	);
}

template <auto &ref, ShaderStage ... Ss>
auto dslbs_for_singlet_group(const stage_wrapper <ref, Ss...> &)
{
	constexpr auto stage_flags = (stage_to_flag(Ss) | ...);
	
	using R = reference_base_of <ref>;

	auto override = contract_desc::overrides[&ref];
	auto packet = resource_packet <R> (override);
	return std::tuple {
		std::array {
			vk::DescriptorSetLayoutBinding()
				.setBinding(0)
				.setDescriptorCount(packet.count)
				.setDescriptorType(packet.type)
				.setStageFlags(stage_flags)
		},
		std::array { packet.flags },
	};
}

template <auto &ref, ShaderStage ... Ss>
auto dslbs_for_contract(const stage_wrapper <ref, Ss...> &wrapper)
{
	using Reference = reference_base_of <ref>;
	if constexpr (is_resource_group_v <Reference>)
		return dslbs_for_resource_group(wrapper);
	else
		return dslbs_for_singlet_group(wrapper);
}

struct _dsl_cache {
	struct Key {
		vk::Device device;
		void *vptr;

		bool operator<(const Key &other) const {
			return (device < other.device)
				? true
				: vptr < other.vptr;
		}
	};

	static inline std::map <Key, vk::DescriptorSetLayout> map;
};

template <auto &ref, ShaderStage ... Ss>
auto one_wrapper_to_dsl(const Device &device, const stage_wrapper <ref, Ss...> &wrapper)
{
	// TODO: should cache all stage flags of the contract at init time?
	// as is, we are unable to call this from the new_descritpor
	// functions, and so cannot do anything before compiling the first pipeline...
	auto key = _dsl_cache::Key(device.logical, &ref);
	if (_dsl_cache::map.contains(key))
		return _dsl_cache::map.at(key);

	auto [dslbs, flags] = dslbs_for_contract(wrapper);
	auto dsl_info = vk::DescriptorSetLayoutCreateInfo()
		// TODO: enable if any configs have it enabled...
		// .setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool)
		.setBindings(dslbs);

	auto binding_flags = vk::DescriptorSetLayoutBindingFlagsCreateInfo()
		.setBindingFlags(flags);
	auto variable_count = [&] {
		for (auto &flag : flags)
			if (flag != vk::DescriptorBindingFlags(0)) return true;
		return false;
	} ();

	if (variable_count)
		dsl_info.setPNext(&binding_flags);

	auto dsl = device.logical.createDescriptorSetLayout(dsl_info);

	_dsl_cache::map.emplace(key, dsl);

	return dsl;
}

template <typename ... Ts>
auto wrappers_to_dsls(const Device &device, const Tlist <Ts...> &)
{
	if constexpr (sizeof...(Ts) == 0)
		return std::array <vk::DescriptorSetLayout, 0> ();
	else
		return std::array { one_wrapper_to_dsl(device, Ts())... };
}

} // namespace rcgp
