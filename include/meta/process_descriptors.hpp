#pragma once

#include "../rhi/device.hpp"
#include "resources.hpp"
#include "resources_collect.hpp"
#include "shader_stage_conversion.hpp"
#include "stage_intrinsics.hpp"

namespace rcgp {

template <typename R>
constexpr auto resource_descriptor_type = []
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
	// else
	// 	return vk::DescriptorType::eUniformBuffer;
} ();

template <auto &ref, ShaderStage ... Ss>
auto dslbs_for_resource_group(const stage_wrapper <ref, Ss...> &)
{
	using R = reference_base_of <ref>;
	using T = R::struct_type;

	constexpr auto stage_flags = (stage_to_flag(Ss) | ...);

	auto fill_one = [&] <size_t I> () {
		using Resource = T::fields::template get <I>;

		return vk::DescriptorSetLayoutBinding()
			.setBinding(I)
			.setDescriptorCount(1)
			.setDescriptorType(resource_descriptor_type <Resource>)
			.setStageFlags(stage_flags);
	};

	return constexpr_for(Is, T::field_count,
		return std::array {
			fill_one.template operator() <Is> ()...
		}
	);
}

template <auto &ref, ShaderStage ... Ss>
auto dslbs_for_singlet_group(const stage_wrapper <ref, Ss...> &)
{
	using R = reference_base_of <ref>;

	constexpr auto stage_flags = (stage_to_flag(Ss) | ...);
	return std::array {
		vk::DescriptorSetLayoutBinding()
			.setBinding(0)
			.setDescriptorCount(1)
			.setDescriptorType(resource_descriptor_type <R>)
			.setStageFlags(stage_flags)
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
	auto key = _dsl_cache::Key(device.logical, &ref);
	if (_dsl_cache::map.contains(key))
		return _dsl_cache::map.at(key);

	auto dslbs = dslbs_for_contract(wrapper);
	auto dsl_info = vk::DescriptorSetLayoutCreateInfo()
		.setBindings(dslbs);

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
