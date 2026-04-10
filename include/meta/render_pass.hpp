#pragma once

#include <optional>
#include <vector>

#include "../rhi/pipelines.hpp"
#include "../rhi/command_buffer.hpp"
#include "../rhi/image.hpp"
#include "commands.hpp"
#include "contract.hpp"
#include "contract_configuration.hpp"

namespace rcgp {

// Convert a Tlist of contracts to a Tlist of effects
template <template <auto &> typename Tag, typename Contracts>
struct target_effects_from;

template <template <auto &> typename Tag, auto &... refs>
struct target_effects_from <Tag, Tlist <contract <refs>...>> {
	using type = Tlist <Tag <refs>...>;
};

// Render target writer/reader (analogous to Dispatcher/Receiver)
template <auto &ref>
struct Writer {
	static constexpr auto &handle = ref;
};

template <auto &ref>
struct Reader {
	static constexpr auto &handle = ref;
};

template <auto &ref>
inline auto writer = Writer <ref> ();

template <auto &ref>
inline auto reader = Reader <ref> ();

TYPE_TRAIT(is_target_writer);
	template <auto &ref>
	TYPE_TRAIT_INCLUDES(is_target_writer, Writer <ref>);

TYPE_TRAIT(is_target_reader);
	template <auto &ref>
	TYPE_TRAIT_INCLUDES(is_target_reader, Reader <ref>);

// Split a flat list of Writer/Reader types into Writes and Reads Tlists
template <typename Writes, typename Reads, typename ... Ts>
struct split_targets;

template <typename Writes, typename Reads>
struct split_targets <Writes, Reads> {
	using writes = Writes;
	using reads = Reads;
};

template <typename Writes, typename Reads, auto &ref, typename ... Rest>
struct split_targets <Writes, Reads, Writer <ref>, Rest...>
	: split_targets <tlist_concat_t <Writes, Tlist <contract <ref>>>, Reads, Rest...> {};

template <typename Writes, typename Reads, auto &ref, typename ... Rest>
struct split_targets <Writes, Reads, Reader <ref>, Rest...>
	: split_targets <Writes, tlist_concat_t <Reads, Tlist <contract <ref>>>, Rest...> {};

// Pass frame: runtime binding of images to render targets
template <typename Writes, typename Reads>
struct PassFrame {
	vk::Rect2D render_area;

	struct TargetBinding {
		Image *image = nullptr;
		vk::ClearValue clear_value;
		bool is_depth = false;
	};

	struct InputBinding {
		Image *image = nullptr;
		bool is_depth = false;
	};

	std::vector <TargetBinding> targets;
	std::vector <InputBinding> inputs;

	template <auto &ref>
	auto &target(Image *image) {
		constexpr bool depth = is_depth_target_v <reference_base_of <ref>>;
		vk::ClearValue cv;
		if constexpr (depth)
			cv = vk::ClearDepthStencilValue(1.0f, 0);
		else
			cv = vk::ClearColorValue(std::array <float, 4> { 0, 0, 0, 0 });
		targets.push_back({ image, cv, depth });
		return *this;
	}

	template <auto &ref>
	auto &target(Image *image, std::array <float, 4> color) {
		targets.push_back({
			image,
			vk::ClearColorValue(color),
			false
		});
		return *this;
	}

	template <auto &ref>
	auto &input(Image *image) {
		constexpr bool depth = is_depth_target_v <reference_base_of <ref>>;
		inputs.push_back({ image, depth });
		return *this;
	}

	auto begin() const {
		auto state = *this;

		auto binder = [state](const CommandBuffer &cmd, SerializationContext &) {
			static constexpr auto color_to_read = BarrierDesc {
				.src_stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
				.src_access = vk::AccessFlagBits2::eColorAttachmentWrite,
				.dst_stage = vk::PipelineStageFlagBits2::eFragmentShader,
				.dst_access = vk::AccessFlagBits2::eShaderSampledRead,
			};

			static constexpr auto depth_to_read = BarrierDesc {
				.src_stage = vk::PipelineStageFlagBits2::eLateFragmentTests,
				.src_access = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
				.dst_stage = vk::PipelineStageFlagBits2::eFragmentShader,
				.dst_access = vk::AccessFlagBits2::eShaderSampledRead,
			};

			static constexpr auto color_to_write = BarrierDesc {
				.src_stage = vk::PipelineStageFlagBits2::eFragmentShader,
				.src_access = vk::AccessFlagBits2::eShaderSampledRead,
				.dst_stage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
				.dst_access = vk::AccessFlagBits2::eColorAttachmentWrite,
			};

			static constexpr auto depth_to_write = BarrierDesc {
				.src_stage = vk::PipelineStageFlagBits2::eFragmentShader,
				.src_access = vk::AccessFlagBits2::eShaderSampledRead,
				.dst_stage = vk::PipelineStageFlagBits2::eEarlyFragmentTests,
				.dst_access = vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
			};

			// Transition read inputs to shader-read-optimal
			for (auto &inp : state.inputs) {
				auto &barrier = inp.is_depth ? depth_to_read : color_to_read;
				cmd.transition(*inp.image,
					vk::ImageLayout::eShaderReadOnlyOptimal,
					barrier);
			}

			// Transition write targets and build attachment infos
			std::vector <vk::RenderingAttachmentInfo> color_atts;
			std::optional <vk::RenderingAttachmentInfo> depth_att;

			for (auto &tgt : state.targets) {
				if (tgt.is_depth) {
					cmd.transition(*tgt.image,
						vk::ImageLayout::eDepthStencilAttachmentOptimal,
						depth_to_write);
					depth_att = vk::RenderingAttachmentInfo()
						.setImageView(tgt.image->view)
						.setImageLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
						.setLoadOp(vk::AttachmentLoadOp::eClear)
						.setStoreOp(vk::AttachmentStoreOp::eStore)
						.setClearValue(tgt.clear_value);
				} else {
					cmd.transition(*tgt.image,
						vk::ImageLayout::eColorAttachmentOptimal,
						color_to_write);
					color_atts.push_back(
						vk::RenderingAttachmentInfo()
							.setImageView(tgt.image->view)
							.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
							.setLoadOp(vk::AttachmentLoadOp::eClear)
							.setStoreOp(vk::AttachmentStoreOp::eStore)
							.setClearValue(tgt.clear_value)
					);
				}
			}

			// Begin rendering
			auto rendering = vk::RenderingInfo()
				.setRenderArea(state.render_area)
				.setLayerCount(1)
				.setColorAttachments(color_atts);

			if (depth_att.has_value())
				rendering.setPDepthAttachment(&depth_att.value());

			cmd.beginRendering(rendering);
		};

		using write_effects = typename target_effects_from <TargetWrite, Writes> ::type;
		using read_effects = typename target_effects_from <TargetRead, Reads> ::type;
		using all_effects = tlist_concat_t <read_effects, write_effects>;
		using Cmds = commands_from_t <false, all_effects>;
		return Cmds { binder };
	}

	auto end() const {
		return Commands <false> {
			[](const CommandBuffer &cmd, SerializationContext &) {
				cmd.endRendering();
			}
		};
	}
};

// Render pass: typed module mapping between render targets
template <typename Writes, typename Reads>
struct RenderPass {
	std::vector <vk::Format> color_formats;
	vk::Format depth_format = vk::Format::eUndefined;

	auto frame(const vk::Rect2D &render_area) const {
		return PassFrame <Writes, Reads> { render_area };
	}

	RenderState render_state() {
		color_formats.clear();
		depth_format = vk::Format::eUndefined;

		auto process = [&] <auto &ref> () {
			using R = reference_base_of <ref>;
			auto fmt = contract_desc::overrides[&ref].format;
			if constexpr (is_depth_target_v <R>)
				depth_format = fmt;
			else
				color_formats.push_back(fmt);
		};

		[] <auto &... refs> (Tlist <contract <refs>...>, auto &process) {
			(process.template operator() <refs> (), ...);
		} (Writes(), process);

		return vk::PipelineRenderingCreateInfo()
			.setColorAttachmentFormats(color_formats)
			.setDepthAttachmentFormat(depth_format);
	}
};

template <typename ... Ts>
auto new_render_pass(Ts...)
{
	using result = split_targets <Tlist <>, Tlist <>, Ts...>;
	return RenderPass <typename result::writes, typename result::reads> ();
}

#define $render_pass(...) rcgp::new_render_pass(__VA_ARGS__)


} // namespace rcgp
