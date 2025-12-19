#pragma once

#include "../rhi/image.hpp"
#include "mirror.hpp"
#include "mirror_buffer.hpp"
#include "resources.hpp"

// TODO: specialization for aggregate
template <reflected T>
struct resource_translator <ResourceGroup <T>> {
	using defer = resource_translator <T>;
	using type = defer::type;
	using value_type = defer::value_type;
	using element_type = defer::element_type;
};

// TODO: needs to be changed
struct SamplerMirror {
	vk::Sampler sampler {};
	vk::ImageView view {};
	vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal;

	vk::DescriptorImageInfo descriptor_info() const {
		return vk::DescriptorImageInfo()
			.setSampler(sampler)
			.setImageView(view)
			.setImageLayout(layout);
	}

	static SamplerMirror from(
		const Device &device,
		const vk::SamplerCreateInfo &sinfo,
		const Image &image,
		vk::ImageLayout layout = vk::ImageLayout::eShaderReadOnlyOptimal
	) {
		SamplerMirror sm;
		sm.sampler = device.logical.createSampler(sinfo);
		sm.view = image.view;
		sm.layout = layout;
		return sm;
	}
};

template <reflected T, template <typename> typename L, vk::VertexInputRate R>
struct resource_translator <AttributeStream <T, L, R>> {
	using buffer = VertexMirrorBuffer <array <T>, L>;
	using type = buffer;
	using value_type = buffer::value_type;
	using element_type = buffer::element_type;
};

template <reflected T, template <typename> typename L>
struct resource_translator <PushConstant <T, L>> {
	using type = TypeMirror <T, L>;
	using value_type = type;
	using element_type = type;
};

template <native_scalar T, size_t D>
struct resource_translator <Sampler <T, D>> {
	using type = SamplerMirror;
	using value_type = type;
	using element_type = std::nullptr_t;
};
