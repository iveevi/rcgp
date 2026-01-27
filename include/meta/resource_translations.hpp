#pragma once

#include "../rhi/image.hpp"
#include "../util/cti.hpp"
#include "mirror.hpp"
#include "mirror_buffer.hpp"
#include "resources.hpp"
#include "static_string.hpp"

namespace rcgp {

namespace resource_layout {

// TODO: laayout engine is a bad name... also need an alias for naturally
// aligned scaffold hints
template <typename T>
struct layout_engine {
	static_error("resource_layout::layout_engine not implemented for type "_ss + $ss_type(T));
};

template <typename List>
struct layout_engine_list;

template <typename ... Ts>
struct layout_engine_list <Tlist <Ts...>> {
	using hint = scaffold_hint <
		Tlist <typename layout_engine <Ts> ::hint...>,
		0
	>;
};

template <aggregate T>
struct layout_engine <T> : layout_engine_list <typename T::fields> {};

template <typename T, int64_t N>
requires (N > 0)
struct layout_engine <array <T, N>> {
	using hint = scaffold_hint <
		std::array <typename layout_engine <T> ::hint, N>,
		0
	>;
};

template <typename T>
struct layout_engine <array <T, -1>> {
	using hint = scaffold_hint <
		unsized_array <typename layout_engine <T> ::hint>,
		0
	>;
};

template <typename T, template <typename> typename L>
struct layout_engine <UniformBuffer <T, L>> {
	using hint = scaffold_hint <ResourceType <UniformBuffer <T, L>>, 0>;
};

template <typename T, template <typename> typename L, GlobalResourceAccess A>
struct layout_engine <StorageBuffer <T, L, A>> {
	using hint = scaffold_hint <ResourceType <StorageBuffer <T, L, A>>, 0>;
};

template <typename T, size_t D>
struct layout_engine <Sampler <T, D>> {
	using hint = scaffold_hint <ResourceType <Sampler <T, D>>, 0>;
};

template <typename T>
struct layout_engine <ResourceGroup <T>> {
	using hint = typename layout_engine <T> ::hint;
};

} // namespace resource_layout

template <typename T>
using ResourceMirror = decltype([] {
	using hint = resource_layout::layout_engine <T> ::hint;
	using type = scaffold_lookup <hint, T, true> ::type;
	return type();
} ());

template <aggregate T>
struct resource_translator <T> {
	using type = ResourceMirror <T>;
	using value_type = type;
	using element_type = std::nullptr_t;
};

template <typename T>
struct resource_translator <ResourceGroup <T>> {
	using type = ResourceMirror <T>;
	using value_type = type;
	using element_type = std::nullptr_t;
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
		sm.layout = (image.layout != vk::ImageLayout::eUndefined)
			? image.layout
			: layout;
		return sm;
	}
};

template <typename T, template <typename> typename L, vk::VertexInputRate R>
struct resource_translator <AttributeStream <T, L, R>> {
	using buffer = VertexMirrorBuffer <array <T>, L>;
	using type = buffer;
	using value_type = buffer::value_type;
	using element_type = buffer::element_type;
};

template <typename T, template <typename> typename L>
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

} // namespace rcgp
