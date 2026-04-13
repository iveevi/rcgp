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

template <auto &ref>
struct Writer {
	static constexpr auto &handle = ref;
};

template <auto &ref>
struct Reader {
	static constexpr auto &handle = ref;
};

template <auto &ref>
struct Sampled {
	static constexpr auto &handle = ref;
};

template <auto &ref>
inline auto writer = Writer <ref> ();

template <auto &ref>
inline auto reader = Reader <ref> ();

template <auto &ref>
inline auto sampled = Sampled <ref> ();

template <typename Writes, typename Reads, typename Sampleds, typename ... Ts>
struct split_targets;

template <typename Writes, typename Reads, typename Sampleds>
struct split_targets <Writes, Reads, Sampleds> {
	using writes = Writes;
	using reads = Reads;
	using sampleds = Sampleds;
};

template <typename Writes, typename Reads, typename Sampleds, auto &ref, typename ... Rest>
struct split_targets <Writes, Reads, Sampleds, Writer <ref>, Rest...>
	: split_targets <tlist_concat_t <Writes, Tlist <contract <ref>>>, Reads, Sampleds, Rest...> {};

template <typename Writes, typename Reads, typename Sampleds, auto &ref, typename ... Rest>
struct split_targets <Writes, Reads, Sampleds, Reader <ref>, Rest...>
	: split_targets <Writes, tlist_concat_t <Reads, Tlist <contract <ref>>>, Sampleds, Rest...> {};

template <typename Writes, typename Reads, typename Sampleds, auto &ref, typename ... Rest>
struct split_targets <Writes, Reads, Sampleds, Sampled <ref>, Rest...>
	: split_targets <Writes, Reads, tlist_concat_t <Sampleds, Tlist <contract <ref>>>, Rest...> {};

template <typename Writes, typename Reads, typename Sampleds>
struct PassFrame {
	vk::Rect2D render_area;

	struct TargetBinding {
		Image *image = nullptr;
		void *target_ref = nullptr;
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
	requires is_depth_target_v <reference_base_of <ref>>
	auto &target(DepthTargetImage *image) {
		targets.push_back({
			image,
			(void *) &ref,
			vk::ClearDepthStencilValue(1.0f, 0),
			true
		});
		return *this;
	}

	template <auto &ref>
	requires (not is_depth_target_v <reference_base_of <ref>>)
	auto &target(ColorTargetImage *image) {
		targets.push_back({
			image,
			(void *) &ref,
			vk::ClearColorValue(std::array <float, 4> { 0, 0, 0, 0 }),
			false
		});
		return *this;
	}

	template <auto &ref>
	requires (not is_depth_target_v <reference_base_of <ref>>)
	auto &target(ColorTargetImage *image, std::array <float, 4> color) {
		targets.push_back({
			image,
			(void *) &ref,
			vk::ClearColorValue(color),
			false
		});
		return *this;
	}

	template <auto &ref>
	requires is_depth_target_v <reference_base_of <ref>>
	auto &input(DepthTargetImage *image) {
		inputs.push_back({ image, true });
		return *this;
	}

	template <auto &ref>
	requires (not is_depth_target_v <reference_base_of <ref>>)
	auto &input(ColorTargetImage *image) {
		inputs.push_back({ image, false });
		return *this;
	}

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

	// Addresses of the sampled<&t> target refs declared by the render pass.
	// Captured here at compile time so the runtime binder can query the sctx
	// registry for the currently bound image of each target.
	static std::vector <std::pair <void *, bool>> sampled_descriptors()
	{
		std::vector <std::pair <void *, bool>> result;
		auto add = [&] <auto &ref> () {
			using R = reference_base_of <ref>;
			result.push_back({ (void *) &ref, is_depth_target_v <R> });
		};
		[&] <auto &... refs> (Tlist <contract <refs>...>) {
			(add.template operator() <refs> (), ...);
		} (Sampleds {});
		return result;
	}

	auto begin() const {
		auto state = *this;

		auto binder = [state](const CommandBuffer &cmd, SerializationContext &sctx) {
			for (auto &inp : state.inputs) {
				auto &barrier = inp.is_depth ? depth_to_read : color_to_read;
				cmd.transition(*inp.image,
					vk::ImageLayout::eShaderReadOnlyOptimal,
					barrier);
			}

			for (auto &[addr, is_depth] : sampled_descriptors()) {
				auto it = sctx.target_images.find(addr);
				if (it == sctx.target_images.end())
					continue;
				auto &reg = it->second;
				cmd.transition(*reg.image,
					vk::ImageLayout::eShaderReadOnlyOptimal,
					reg.is_depth ? depth_to_read : color_to_read);
			}

			std::vector <vk::RenderingAttachmentInfo> color_atts;
			std::optional <vk::RenderingAttachmentInfo> depth_att;

			for (auto &tgt : state.targets) {
				sctx.target_images[tgt.target_ref] = TargetRegistration {
					.image = tgt.image,
					.is_depth = tgt.is_depth,
				};

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
		using sampled_read_effects = typename target_effects_from <TargetRead, Sampleds> ::type;
		using sampled_decl_effects = typename target_effects_from <DeclaresSampled, Sampleds> ::type;
		using all_effects = tlist_concat_t <
			read_effects,
			sampled_read_effects,
			sampled_decl_effects,
			write_effects
		>;
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

template <typename Writes, typename Reads, typename Sampleds>
struct RenderPass {
	using writes = Writes;
	using reads = Reads;
	using sampleds = Sampleds;

	std::vector <vk::Format> color_formats;
	vk::Format depth_format = vk::Format::eUndefined;

	auto frame(const vk::Rect2D &render_area) const {
		return PassFrame <Writes, Reads, Sampleds> { render_area };
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
	using result = split_targets <Tlist <>, Tlist <>, Tlist <>, Ts...>;
	return RenderPass <
		typename result::writes,
		typename result::reads,
		typename result::sampleds
	> ();
}

#define $render_pass(...) rcgp::new_render_pass(__VA_ARGS__)


} // namespace rcgp
