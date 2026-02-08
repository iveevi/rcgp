#pragma once

#include <array>
#include <map>
#include <vector>

#include "vk.hpp"
#include "../util/cti.hpp"

namespace rcgp {

struct Attachments {
	std::vector <vk::AttachmentDescription> descriptions;
	std::map <std::string, uint32_t> mapping;

	vk::AttachmentDescription &operator[](const std::string &key) &;

	vk::AttachmentReference reference(const std::string &key, vk::ImageLayout layout) const;
};

template <size_t N>
struct color_attachments : std::array <vk::AttachmentReference, N> {
	template <typename ... Ts>
	requires (sizeof...(Ts) == N)
	color_attachments(Ts ... args)
		: std::array <vk::AttachmentReference, N> {
			Tas <vk::AttachmentReference> (args)...
		} {}
};

template <typename ... Ts>
color_attachments(Ts...) -> color_attachments <sizeof...(Ts)>;

color_attachments() -> color_attachments <0>;

template <size_t N>
struct depth_attachments : std::array <vk::AttachmentReference, N> {
	template <typename ... Ts>
	requires (sizeof...(Ts) == N)
	depth_attachments(Ts ... args)
		: std::array <vk::AttachmentReference, N> {
			Tas <vk::AttachmentReference> (args)...
		} {}
};

template <typename ... Ts>
depth_attachments(Ts...) -> depth_attachments <sizeof...(Ts)>;

depth_attachments() -> depth_attachments <0>;

template <size_t N>
struct input_attachments : std::array <vk::AttachmentReference, N> {
	template <typename ... Ts>
	requires (sizeof...(Ts) == N)
	input_attachments(Ts ... args)
		: std::array <vk::AttachmentReference, N> {
			Tas <vk::AttachmentReference> (args)...
		} {}
};

template <typename ... Ts>
input_attachments(Ts...) -> input_attachments <sizeof...(Ts)>;

input_attachments() -> input_attachments <0>;

template <size_t Colors, size_t Depths, size_t Resolves, size_t Inputs>
struct Subpass : vk::SubpassDescription {
	color_attachments <Colors> colors {};
	depth_attachments <Depths> depths {};
	color_attachments <Resolves> resolves {};
	input_attachments <Inputs> inputs {};

	Subpass(
		color_attachments <Colors> colors_,
		depth_attachments <Depths> depths_ = {},
		color_attachments <Resolves> resolves_ = {},
		input_attachments <Inputs> inputs_ = {}
	)
		: colors(colors_), depths(depths_), resolves(resolves_), inputs(inputs_)
	{
		pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

		if constexpr (Colors > 0) {
			colorAttachmentCount = colors.size();
			pColorAttachments = colors.data();
		}

		if constexpr (Inputs > 0) {
			pInputAttachments = inputs.data();
			inputAttachmentCount = inputs.size();
		}

		if constexpr (Depths > 0) {
			pDepthStencilAttachment = depths.data();
		}

		if constexpr (Resolves > 0) {
			pResolveAttachments = resolves.data();
		}
	}
};

template <size_t C, size_t D, size_t R, size_t I>
auto subpass(
	color_attachments <C> colors,
	depth_attachments <D> depths,
	color_attachments <R> resolves,
	input_attachments <I> inputs
)
{
	return Subpass <C, D, R, I> (colors, depths, resolves, inputs);
}

template <typename ... Ts>
struct RenderPass : vk::RenderPass {};

} // namespace rcgp
