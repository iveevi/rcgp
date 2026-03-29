#pragma once

#include <map>

#include <vulkan/vulkan.hpp>

#include "contract.hpp"
#include "dynamic.hpp"
#include "resources.hpp"

namespace rcgp {

// Per-contract override configuration
struct contract_desc {
	vk::DescriptorType type;
	vk::DescriptorBindingFlags flags = vk::DescriptorBindingFlags(0);
	uint32_t count = 1;
	vk::Format format = vk::Format::eUndefined;

	static inline std::map <void *, contract_desc> overrides;
};

template <auto &ref>
struct contract_config {
	using R = reference_base_of <ref>;

	static void reserve(uint32_t size) {
		static_assert(is_dynamic_v <R>, "reserve is only available for variable sized resources");
		contract_desc::overrides[&ref].count = size;
	}

	static void update_after_bind(bool enable) {
		auto &flags = contract_desc::overrides[&ref].flags;
		if (enable)
			flags |= vk::DescriptorBindingFlagBits::eUpdateAfterBind;
		else
			flags &= ~vk::DescriptorBindingFlagBits::eUpdateAfterBind;
	}

	static void format(vk::Format fmt) {
		static_assert(is_render_target_v <R>, "format is only available for render targets");
		contract_desc::overrides[&ref].format = fmt;
	}
};

#define $config(...) rcgp::contract_config <__VA_ARGS__>

} // namespace rcgp
