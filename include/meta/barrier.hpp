#pragma once

#include <array>

#include <vulkan/vulkan.hpp>

#include "resources.hpp"
#include "reference.hpp"
#include "reference_introspection.hpp"

namespace rcgp {

enum class ResourceAccess {
	eNone,
	eRead,
	eWrite,
	eReadWrite,
};

enum class PipelineStage {
	eNone,
	eCompute,
	eVertex,
	eFragment,
	eTransfer,
	eHost,
};

template <PipelineStage S, ResourceAccess A>
struct Phase {
	static constexpr PipelineStage stage = S;
	static constexpr ResourceAccess access = A;
};

template <PipelineStage S>
struct StageTag {
	static constexpr PipelineStage value = S;
};

template <ResourceAccess A>
struct AccessTag {
	static constexpr ResourceAccess value = A;
};

template <PipelineStage S, ResourceAccess A>
struct PhaseTag {
	static constexpr PipelineStage stage = S;
	static constexpr ResourceAccess access = A;
};

inline constexpr StageTag <PipelineStage::eCompute> ComputeStage {};
inline constexpr StageTag <PipelineStage::eVertex> VertexStage {};
inline constexpr StageTag <PipelineStage::eFragment> FragmentStage {};
inline constexpr StageTag <PipelineStage::eTransfer> TransferStage {};
inline constexpr StageTag <PipelineStage::eHost> HostStage {};

inline constexpr AccessTag <ResourceAccess::eRead> ReadAccess {};
inline constexpr AccessTag <ResourceAccess::eWrite> WriteAccess {};
inline constexpr AccessTag <ResourceAccess::eReadWrite> ReadWriteAccess {};

inline constexpr PhaseTag <PipelineStage::eCompute, ResourceAccess::eRead> ComputeRead {};
inline constexpr PhaseTag <PipelineStage::eCompute, ResourceAccess::eWrite> ComputeWrite {};
inline constexpr PhaseTag <PipelineStage::eCompute, ResourceAccess::eReadWrite> ComputeReadWrite {};
inline constexpr PhaseTag <PipelineStage::eHost, ResourceAccess::eRead> HostRead {};


constexpr vk::PipelineStageFlags2 to_vk_stage(PipelineStage stage)
{
	using enum PipelineStage;
	switch (stage) {
	case eCompute: return vk::PipelineStageFlagBits2::eComputeShader;
	case eVertex: return vk::PipelineStageFlagBits2::eVertexShader;
	case eFragment: return vk::PipelineStageFlagBits2::eFragmentShader;
	case eTransfer: return vk::PipelineStageFlagBits2::eTransfer;
	case eHost: return vk::PipelineStageFlagBits2::eHost;
	case eNone: break;
	}
	return {};
}

constexpr vk::AccessFlags2 to_vk_access(PipelineStage stage, ResourceAccess access)
{
	switch (stage) {
	case PipelineStage::eCompute:
	case PipelineStage::eVertex:
	case PipelineStage::eFragment:
		switch (access) {
		case ResourceAccess::eRead: return vk::AccessFlagBits2::eShaderRead;
		case ResourceAccess::eWrite: return vk::AccessFlagBits2::eShaderWrite;
		case ResourceAccess::eReadWrite: return vk::AccessFlagBits2::eShaderRead | vk::AccessFlagBits2::eShaderWrite;
		case ResourceAccess::eNone: break;
		}
		break;
	case PipelineStage::eTransfer:
		switch (access) {
		case ResourceAccess::eRead: return vk::AccessFlagBits2::eTransferRead;
		case ResourceAccess::eWrite: return vk::AccessFlagBits2::eTransferWrite;
		case ResourceAccess::eReadWrite: return vk::AccessFlagBits2::eTransferRead | vk::AccessFlagBits2::eTransferWrite;
		case ResourceAccess::eNone: break;
		}
		break;
	case PipelineStage::eHost:
		switch (access) {
		case ResourceAccess::eRead: return vk::AccessFlagBits2::eHostRead;
		case ResourceAccess::eWrite: return vk::AccessFlagBits2::eHostWrite;
		case ResourceAccess::eReadWrite: return vk::AccessFlagBits2::eHostRead | vk::AccessFlagBits2::eHostWrite;
		case ResourceAccess::eNone: break;
		}
		break;
	case PipelineStage::eNone:
		break;
	}
	return {};
}

template <typename T>
concept buffer_resource = requires(const T &resource) {
	{ resource.descriptor_info() } -> std::same_as <vk::DescriptorBufferInfo>;
};

template <typename T>
consteval size_t buffer_bindings_for_group()
{
	using Structure = T::value_type;
	static_assert(aggregate <Structure>);
	size_t count = 0;
	auto count_one = [&] <size_t I> () {
		using Field = Structure::reflection::template field_type <I>;
		if constexpr (buffer_resource <Field>)
			count++;
	};
	constexpr_for(Is, Structure::reflection::field_count,
		(count_one.template operator() <Is> (), ...)
	);
	return count;
}

template <auto &ref, typename SrcPhase, typename DstPhase>
struct Barrier {
	using Reference = reference_base_t <ref>;
	using src_phase = SrcPhase;
	using dst_phase = DstPhase;

	static constexpr size_t count = 1;
	vk::BufferMemoryBarrier2 barrier;

	Barrier() = default;

	explicit Barrier(const ResourceTypeFor <ref> &resource) {
		static_assert(buffer_resource <ResourceTypeFor <ref>>,
			"Barrier requires a buffer-backed resource");
		auto info = resource.descriptor_info();
		barrier
			.setBuffer(info.buffer)
			.setOffset(info.offset)
			.setSize(info.range);
		barrier
			.setSrcStageMask(to_vk_stage(SrcPhase::stage))
			.setSrcAccessMask(to_vk_access(SrcPhase::stage, SrcPhase::access))
			.setDstStageMask(to_vk_stage(DstPhase::stage))
			.setDstAccessMask(to_vk_access(DstPhase::stage, DstPhase::access));
	}

	void write_to(std::span <vk::BufferMemoryBarrier2> out, size_t &idx) const {
		out[idx++] = barrier;
	}
};

template <auto &ref, typename SrcPhase, typename DstPhase>
requires is_resource_group_v <reference_base_t <ref>>
struct Barrier <ref, SrcPhase, DstPhase> {
	using Reference = reference_base_t <ref>;

	static constexpr size_t count = buffer_bindings_for_group <Reference> ();
	std::array <vk::BufferMemoryBarrier2, count> barriers {};

	Barrier() = default;

	explicit Barrier(const ResourceTypeFor <ref> &resource) {
		using Structure = Reference::value_type;
		static_assert(aggregate <Structure>);

		size_t idx = 0;
		auto bind_one = [&] <size_t I> () {
			using Field = Structure::reflection::template field_type <I>;
			if constexpr (buffer_resource <Field>) {
				auto info = resource.template get <I> ().descriptor_info();
				barriers[idx]
					.setBuffer(info.buffer)
					.setOffset(info.offset)
					.setSize(info.range)
					.setSrcStageMask(to_vk_stage(SrcPhase::stage))
					.setSrcAccessMask(to_vk_access(SrcPhase::stage, SrcPhase::access))
					.setDstStageMask(to_vk_stage(DstPhase::stage))
					.setDstAccessMask(to_vk_access(DstPhase::stage, DstPhase::access));
				idx++;
			}
		};

		constexpr_for(Is, Structure::reflection::field_count,
			(bind_one.template operator() <Is> (), ...)
		);
	}

	void write_to(std::span <vk::BufferMemoryBarrier2> out, size_t &idx) const {
		for (auto &entry : barriers)
			out[idx++] = entry;
	}
};

template <auto &ref, PipelineStage SS, ResourceAccess SA, PipelineStage DS, ResourceAccess DA>
auto barrier(const ResourceTypeFor <ref> &resource)
{
	return Barrier <ref, Phase <SS, SA>, Phase <DS, DA>> (resource);
}

template <auto &ref, PipelineStage SS, ResourceAccess SA, PipelineStage DS, ResourceAccess DA>
auto barrier(
	const ResourceTypeFor <ref> &resource,
	PhaseTag <SS, SA>,
	PhaseTag <DS, DA>
)
{
	return Barrier <ref, Phase <SS, SA>, Phase <DS, DA>> (resource);
}

template <auto &ref, PipelineStage SS, ResourceAccess SA, PipelineStage DS, ResourceAccess DA>
auto barrier(
	const ResourceTypeFor <ref> &resource,
	StageTag <SS>, AccessTag <SA>,
	StageTag <DS>, AccessTag <DA>
)
{
	return Barrier <ref, Phase <SS, SA>, Phase <DS, DA>> (resource);
}

} // namespace rcgp
