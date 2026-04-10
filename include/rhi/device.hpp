#pragma once

#include <concepts>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#include "session.hpp"
#include "render_pass.hpp"
#include "queue.hpp"

namespace rcgp {

// Forward declarations
template <bool Live, typename ... Effects>
struct Commands;

struct CommandBuffer;
struct CommandPool;
struct CommandStream;
struct DescriptorPool;
struct PresentationSynchronizer;
struct TimestampQueryPool;
struct TimestampQueryResult;
struct Window;
struct AccelerationStructure;
struct Buffer;
struct XlasInstance;
struct ShaderBindingTable;

template <typename T, template <typename> typename L>
struct BufferAddress;

template <typename T, template <typename> typename L, vk::BufferUsageFlagBits F>
struct MirrorBuffer;

template <auto &ref>
struct UnboundDescriptor;

template <auto &ref>
struct DescriptorWrite;

vk::DebugUtilsObjectNameInfoEXT name_info(const vk::CommandBuffer &);

struct Device {
	vk::Device logical;
	vk::PhysicalDevice physical;
	vk::PhysicalDeviceMemoryProperties properties;
	vk::detail::DispatchLoaderDynamic loader;
	std::map <std::string, Queue> queues;

	void destroy() {
		logical.destroy();
	}

	void set_name(const auto &object, const std::string &name) {
		auto info = name_info(object)
			.setPObjectName(name.c_str());
		logical.setDebugUtilsObjectNameEXT(info, loader);
	}

	auto find_memory_type(uint32_t filter, vk::MemoryPropertyFlags flags) const -> uint32_t;

	auto get_address(const Buffer &buffer) const -> vk::DeviceAddress;
	auto get_address(const AccelerationStructure &as) const -> vk::DeviceAddress;

	template <typename T, template <typename> typename L, vk::BufferUsageFlagBits F>
	requires (bool(F & vk::BufferUsageFlagBits::eShaderDeviceAddress))
	auto address(const MirrorBuffer <T, L, F> &buffer) const -> BufferAddress <T, L>;

	// TODO: untemplate
	template <typename Extent>
	auto new_framebuffer(
		const vk::RenderPass &render_pass,
		const std::span <const vk::ImageView> &attachments,
		const Extent &extent,
		uint32_t layers = 1
	) const -> vk::Framebuffer {
		static_assert(std::same_as <Extent, vk::Extent2D>
			|| std::same_as <Extent, vk::Extent3D>);

		auto fb_info = vk::FramebufferCreateInfo()
			.setRenderPass(render_pass)
			.setAttachments(attachments)
			.setWidth(extent.width)
			.setHeight(extent.height)
			.setLayers(layers);

		return logical.createFramebuffer(fb_info);
	}

	auto new_command_buffers(const CommandPool &cpool, size_t count) const -> std::vector <CommandBuffer>;
	auto new_command_streams(const CommandPool &cpool, size_t count) const -> std::vector <CommandStream>;
	auto new_command_modules(const CommandPool &cpool, size_t count) const -> std::vector <Commands <true>>;
	auto new_descriptor_sets(const DescriptorPool &dpool, const vk::ArrayProxy <vk::DescriptorSetLayout> &dsls) const -> std::vector <vk::DescriptorSet>;

	template <auto &ref>
	auto new_descriptor(const DescriptorPool &dpool) const -> UnboundDescriptor <ref>;
	
	template <auto &ref>
	auto new_descriptor(const DescriptorPool &dpool, uint32_t max_count) const -> UnboundDescriptor <ref>;

	template <auto &...refs>
	[[nodiscard]] auto update_descriptors(DescriptorWrite <refs> &&... dwpairs) const;

	auto new_shader_module(std::span <const uint32_t> spirv) const -> vk::ShaderModule {
		auto info = vk::ShaderModuleCreateInfo()
			.setCodeSize(spirv.size() * sizeof(uint32_t))
			.setPCode(spirv.data());

		return logical.createShaderModule(info);
	}

	auto new_render_pass(
		const Attachments &attachments,
		std::span <const Subpass> subpasses
	) const -> vk::RenderPass;

	std::optional <uint32_t> acquire_next_frame(
		Window &window,
		PresentationSynchronizer &sync,
		uint64_t timeout = UINT64_MAX
	) const;
	bool rebuild_swapchain(Window &window) const;

	// Acceleration structures
	auto build_blas(
		const CommandBuffer &cmd,
		const Buffer &vertices, const Buffer &indices,
		size_t vertex_count, size_t primitive_count
	) const -> std::tuple <AccelerationStructure, Buffer>;

	auto build_tlas(
		const CommandBuffer &cmd,
		const std::vector <XlasInstance> &instances
	) const -> std::tuple <AccelerationStructure, Buffer>;

	// Shader binding table
	auto build_sbt(
		vk::Pipeline pipeline,
		uint32_t miss_count,
		uint32_t hit_count
	) const -> ShaderBindingTable;

	// One-shot command submission
	template <typename F>
	requires std::is_invocable_v <F, const CommandBuffer &>
	auto one_shot(const Queue &queue, const CommandPool &cpool, F &&ftn) const;

	// Timestamp pool
	TimestampQueryPool new_timestamp_pool(vk::QueryResultFlags flags, size_t count) const;
	TimestampQueryResult get_timestamp_results(const TimestampQueryPool &tqpool) const;

	struct Options {
		std::vector <const char *> extensions;
		std::map <std::string, vk::QueueFlags> queues;
		void *pNext;
	};

	static Device from(
		const Session &session,
		vk::detail::DispatchLoaderDynamic &dld,
		const Options &info
	);
};

} // namespace rcgp
