#pragma once

#include <array>

#include "contract.hpp"
#include "resources.hpp"
#include "witnesses.hpp"

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


constexpr VkPipelineStageFlags2 to_vk_stage(PipelineStage stage)
{
	using enum PipelineStage;
	switch (stage) {
	case eCompute: return VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	case eVertex: return VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
	case eFragment: return VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	case eTransfer: return VK_PIPELINE_STAGE_2_TRANSFER_BIT;
	case eHost: return VK_PIPELINE_STAGE_2_HOST_BIT;
	case eNone: break;
	}
	return 0;
}

constexpr VkAccessFlags2 to_vk_access(PipelineStage stage, ResourceAccess access)
{
	switch (stage) {
	case PipelineStage::eCompute:
	case PipelineStage::eVertex:
	case PipelineStage::eFragment:
		switch (access) {
		case ResourceAccess::eRead: return VK_ACCESS_2_SHADER_READ_BIT;
		case ResourceAccess::eWrite: return VK_ACCESS_2_SHADER_WRITE_BIT;
		case ResourceAccess::eReadWrite: return VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
		case ResourceAccess::eNone: break;
		}
		break;
	case PipelineStage::eTransfer:
		switch (access) {
		case ResourceAccess::eRead: return VK_ACCESS_2_TRANSFER_READ_BIT;
		case ResourceAccess::eWrite: return VK_ACCESS_2_TRANSFER_WRITE_BIT;
		case ResourceAccess::eReadWrite: return VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
		case ResourceAccess::eNone: break;
		}
		break;
	case PipelineStage::eHost:
		switch (access) {
		case ResourceAccess::eRead: return VK_ACCESS_2_HOST_READ_BIT;
		case ResourceAccess::eWrite: return VK_ACCESS_2_HOST_WRITE_BIT;
		case ResourceAccess::eReadWrite: return VK_ACCESS_2_HOST_READ_BIT | VK_ACCESS_2_HOST_WRITE_BIT;
		case ResourceAccess::eNone: break;
		}
		break;
	case PipelineStage::eNone:
		break;
	}
	return 0;
}

template <typename T>
concept buffer_resource = requires(const T &resource) {
	{ resource.descriptor_info() } -> std::same_as <VkDescriptorBufferInfo>;
};

template <typename T>
consteval size_t buffer_bindings_for_group()
{
	using Structure = T::value_type;
	static_assert(user_defined <Structure>);
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
	using Reference = reference_base_of <ref>;
	using src_phase = SrcPhase;
	using dst_phase = DstPhase;

	static constexpr size_t count = 1;
	VkBufferMemoryBarrier2 barrier;

	Barrier() = default;

	explicit Barrier(const ResourceTypeFor <ref> &resource) {
		static_assert(buffer_resource <ResourceTypeFor <ref>>,
			"Barrier requires a buffer-backed resource");
		auto info = resource.descriptor_info();
		barrier.buffer = info.buffer;
		barrier.offset = info.offset;
		barrier.size = info.range;
		barrier.srcStageMask = to_vk_stage(SrcPhase::stage);
		barrier.srcAccessMask = to_vk_access(SrcPhase::stage, SrcPhase::access);
		barrier.dstStageMask = to_vk_stage(DstPhase::stage);
		barrier.dstAccessMask = to_vk_access(DstPhase::stage, DstPhase::access);
	}

	void write_to(std::span <VkBufferMemoryBarrier2> out, size_t &idx) const {
		out[idx++] = barrier;
	}
};

template <auto &ref, typename SrcPhase, typename DstPhase>
requires is_resource_group_v <reference_base_of <ref>>
struct Barrier <ref, SrcPhase, DstPhase> {
	using Reference = reference_base_of <ref>;

	static constexpr size_t count = buffer_bindings_for_group <Reference> ();
	std::array <VkBufferMemoryBarrier2, count> barriers {};

	Barrier() = default;

	explicit Barrier(const ResourceTypeFor <ref> &resource) {
		using Structure = Reference::value_type;
		static_assert(user_defined <Structure>);

		size_t idx = 0;
		auto bind_one = [&] <size_t I> () {
			using Field = Structure::reflection::template field_type <I>;
			if constexpr (buffer_resource <Field>) {
				auto info = resource.template get <I> ().descriptor_info();
				auto &barrier = barriers[idx++];
				// TODO: this is common...
				barrier.buffer = info.buffer;
				barrier.offset = info.offset;
				barrier.size = info.range;
				barrier.srcStageMask = to_vk_stage(SrcPhase::stage);
				barrier.srcAccessMask = to_vk_access(SrcPhase::stage, SrcPhase::access);
				barrier.dstStageMask = to_vk_stage(DstPhase::stage);
				barrier.dstAccessMask = to_vk_access(DstPhase::stage, DstPhase::access);
			}
		};

		constexpr_for(Is, Structure::reflection::field_count,
			(bind_one.template operator() <Is> (), ...)
		);
	}

	void write_to(std::span <VkBufferMemoryBarrier2> out, size_t &idx) const {
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
