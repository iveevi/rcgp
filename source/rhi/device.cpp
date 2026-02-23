#include <ranges>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <print>
#include <set>

#include <GLFW/glfw3.h>

#include "meta/command_stream.hpp"
#include "rhi/command_buffer.hpp"
#include "rhi/command_pool.hpp"
#include "rhi/descriptor_pool.hpp"
#include "rhi/device.hpp"
#include "rhi/timestamp_pool.hpp"
#include "rhi/window.hpp"
#include "util/cti.hpp"

namespace rcgp {

static vk::Extent2D framebuffer_extent_of(GLFWwindow *window)
{
	int width = 0;
	int height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	return vk::Extent2D(uint32_t(width), uint32_t(height));
}

static vk::Extent2D choose_swapchain_extent(
	const vk::SurfaceCapabilitiesKHR &capabilities,
	vk::Extent2D framebuffer_extent
)
{
	if (capabilities.currentExtent.width != std::numeric_limits <uint32_t> ::max())
		return capabilities.currentExtent;

	return vk::Extent2D(
		std::clamp(
			framebuffer_extent.width,
			capabilities.minImageExtent.width,
			capabilities.maxImageExtent.width
		),
		std::clamp(
			framebuffer_extent.height,
			capabilities.minImageExtent.height,
			capabilities.maxImageExtent.height
		)
	);
}

static uint32_t choose_swapchain_image_count(const vk::SurfaceCapabilitiesKHR &capabilities)
{
	auto count = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0)
		count = std::min(count, capabilities.maxImageCount);
	return count;
}

auto Device::find_memory_type(uint32_t filter, vk::MemoryPropertyFlags flags) const
	-> uint32_t
{
	for (uint32_t i = 0; i < properties.memoryTypeCount; ++i) {
		bool type_supported = filter & (1u << i);
		bool properties_supported = (properties.memoryTypes[i].propertyFlags & flags) == flags;
		if (type_supported && properties_supported)
			return i;
	}

	std::println(std::cerr, "no compatible memory type for buffer");
	std::abort();
}

auto Device::new_command_buffers(const CommandPool &cpool, size_t count) const
	-> std::vector <CommandBuffer>
{
	auto info = vk::CommandBufferAllocateInfo()
		.setCommandBufferCount(count)
		.setCommandPool(cpool);

	return logical.allocateCommandBuffers(info)
		| std::views::transform([&] (auto x) { return CommandBuffer(x, &loader); })
		| std::ranges::to <std::vector> ();
}

auto Device::new_command_streams(const CommandPool &cpool, size_t count) const
	-> std::vector <CommandStream>
{
	auto info = vk::CommandBufferAllocateInfo()
		.setCommandBufferCount(count)
		.setCommandPool(cpool);

	return logical.allocateCommandBuffers(info)
		| std::views::transform([&] (auto x) { return CommandStream(x, &loader); })
		| std::ranges::to <std::vector> ();
}
	
auto Device::new_descriptor_sets(const DescriptorPool &dpool, const vk::ArrayProxy <vk::DescriptorSetLayout> &dsls) const
	-> std::vector <vk::DescriptorSet>
{
	auto info = vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(dpool)
		.setSetLayouts(dsls);

	return logical.allocateDescriptorSets(info);
}

auto Device::new_render_pass(
	const Attachments &attachments,
	std::span <const Subpass> subpasses
) const -> vk::RenderPass
{
	auto descriptions = std::vector <vk::SubpassDescription> {};
	descriptions.reserve(subpasses.size());

	for (const auto &subpass : subpasses) {
		auto description = vk::SubpassDescription()
			.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);

		if (!subpass.colors.empty())
			description.setColorAttachments(subpass.colors);

		if (!subpass.depths.empty())
			description.setPDepthStencilAttachment(&subpass.depths.front());

		if (!subpass.resolves.empty())
			description.setResolveAttachments(subpass.resolves);

		if (!subpass.inputs.empty())
			description.setInputAttachments(subpass.inputs);

		descriptions.push_back(description);
	}

	auto rp_info = vk::RenderPassCreateInfo()
		.setAttachments(attachments.descriptions)
		.setSubpasses(descriptions);

	return logical.createRenderPass(rp_info);
}
	
void Device::wait_for_frame(const Frame &frame, uint64_t timeout) const
{
	auto wait = logical.waitForFences(frame.fence, true, timeout);
	if (wait != vk::Result::eSuccess && wait != vk::Result::eTimeout) {
		std::println(std::cerr, "waitForFences failed ({})", vk::to_string(wait));
		std::abort();
	}
}

void Device::reset_frame_fence(const Frame &frame) const
{
	logical.resetFences(frame.fence);
}

FrameAcquireStatus Device::acquire_image_for_frame(Window &window, Frame &frame, uint64_t timeout) const
{
	auto acq = logical.acquireNextImageKHR(
		frame.swapchain,
		timeout,
		frame.presented,
		nullptr
	);

	if (acq.result == vk::Result::eSuccess) {
		frame.image_index = acq.value;
		return FrameAcquireStatus::Ok;
	}

	if (acq.result == vk::Result::eSuboptimalKHR) {
		frame.image_index = acq.value;
		window.swapchain_rebuild_requested = true;
		return FrameAcquireStatus::Suboptimal;
	}

	if (acq.result == vk::Result::eErrorOutOfDateKHR) {
		window.swapchain_rebuild_requested = true;
		return FrameAcquireStatus::OutOfDate;
	}

	std::println(std::cerr, "acquireNextImageKHR failed ({})", vk::to_string(acq.result));
	std::abort();
}

bool Device::rebuild_swapchain(Window &window) const
{
	auto framebuffer = framebuffer_extent_of(window.handle);
	if (framebuffer.width == 0 || framebuffer.height == 0)
		return false;

	auto capabilities = physical.getSurfaceCapabilitiesKHR(window.surface);
	auto new_extent = choose_swapchain_extent(capabilities, framebuffer);
	if (new_extent.width == 0 || new_extent.height == 0)
		return false;

	auto new_swapchain_info = vk::SwapchainCreateInfoKHR()
		.setImageArrayLayers(1)
		.setImageColorSpace(vk::ColorSpaceKHR::eSrgbNonlinear)
		.setImageExtent(new_extent)
		.setImageFormat(window.format)
		.setMinImageCount(choose_swapchain_image_count(capabilities))
		.setOldSwapchain(window.swapchain)
		.setPresentMode(window.present_mode)
		.setImageUsage(
			vk::ImageUsageFlagBits::eColorAttachment
			| vk::ImageUsageFlagBits::eTransferDst
		)
		.setSurface(window.surface);

	auto new_swapchain = logical.createSwapchainKHR(new_swapchain_info);
	auto swapchain_images = logical.getSwapchainImagesKHR(new_swapchain);

	auto new_images = std::vector <Image> ();
	new_images.reserve(swapchain_images.size());
	for (auto &handle : swapchain_images) {
		Image image;
		image.device = logical;
		image.handle = handle;
		image.layout = vk::ImageLayout::eUndefined;
		image.description.extent = vk::Extent3D(new_extent, 1);
		image.description.format = window.format;
		image.description.aspect = vk::ImageAspectFlagBits::eColor;

		auto view_info = vk::ImageViewCreateInfo()
			.setImage(handle)
			.setViewType(vk::ImageViewType::e2D)
			.setSubresourceRange(
				vk::ImageSubresourceRange()
					.setAspectMask(vk::ImageAspectFlagBits::eColor)
					.setBaseArrayLayer(0)
					.setBaseMipLevel(0)
					.setLayerCount(1)
					.setLevelCount(1)
			)
			.setFormat(window.format);

		image.view = logical.createImageView(view_info);
		new_images.push_back(image);
	}

	auto new_frames_in_flight = new_images.size();
	auto fence_info = vk::FenceCreateInfo()
		.setFlags(vk::FenceCreateFlagBits::eSignaled);
	auto semaphore_info = vk::SemaphoreCreateInfo();

	auto new_image_rendered = std::vector <vk::Semaphore> (new_images.size());
	for (auto &semaphore : new_image_rendered)
		semaphore = logical.createSemaphore(semaphore_info);

	auto new_frames = std::vector <Frame> (new_frames_in_flight);
	for (auto &frame : new_frames) {
		frame.swapchain = new_swapchain;
		frame.fence = logical.createFence(fence_info);
		frame.presented = logical.createSemaphore(semaphore_info);
		frame.extent = new_extent;
	}

	for (auto &frame : window.frames) {
		if (frame.fence)
			logical.destroyFence(frame.fence);
		if (frame.presented)
			logical.destroySemaphore(frame.presented);
	}
	for (auto &semaphore : window.image_rendered) {
		if (semaphore)
			logical.destroySemaphore(semaphore);
	}

	for (auto &image : window.images)
		image.destroy();

	if (window.swapchain)
		logical.destroySwapchainKHR(window.swapchain);

	window.swapchain = new_swapchain;
	window.images = std::move(new_images);
	window.image_rendered = std::move(new_image_rendered);
	window.frames = std::move(new_frames);
	window.frames_in_flight = new_frames_in_flight;
	window.frame_index = window.frames_in_flight > 0 ? (window.frames_in_flight - 1) : 0;
	window.swapchain_extent = new_extent;
	window.swapchain_rebuild_requested = false;
	return true;
}

TimestampQueryPool Device::new_timestamp_pool(vk::QueryResultFlags flags, size_t count) const
{
	TimestampQueryPool tqpool;

	auto info = vk::QueryPoolCreateInfo()
		.setQueryCount(count)
		.setQueryType(vk::QueryType::eTimestamp);


	auto properties = physical.getProperties();
	tqpool.period = properties.limits.timestampPeriod;
	tqpool.handle = logical.createQueryPool(info);
	tqpool.flags = flags;
	tqpool.count = count;

	return tqpool;
}

TimestampQueryResult Device::get_timestamp_results(const TimestampQueryPool &tqpool) const
{
	auto su64 = sizeof(uint64_t);
	auto stride = (tqpool.flags & vk::QueryResultFlagBits::eWithAvailability) ? (2 * su64) : su64;
	auto data_size = tqpool.count * stride;
	auto queries = logical.getQueryPoolResults <uint64_t> (
		tqpool.handle,
		0, tqpool.count,
		data_size, stride,
		vk::QueryResultFlagBits::e64 | tqpool.flags
	);

	if (queries.has_value())
		return { queries.value, tqpool.flags, tqpool.period };
	else
		return { {}, tqpool.flags, tqpool.period };
}

Device Device::from(
	const Session &session,
	vk::detail::DispatchLoaderDynamic &dld,
	const Options &options
)
{
	Device device;

	device.physical = session.handle.enumeratePhysicalDevices().front();
	device.properties = device.physical.getMemoryProperties();

	auto queue_families = device.physical.getQueueFamilyProperties();
	auto queue_allocation = std::set <int32_t> ();

	auto queue_family_index = [&](const vk::QueueFlags &flags) -> int32_t {
		// Prefer exact match
		for (const auto &[i, family] : std::views::enumerate(queue_families)) {
			if (family.queueFlags == flags)
				return i;
		}
		
		// Otherwise, prefer first non-allocated one
		int32_t first_applicable = -1;
		for (const auto &[i, family] : std::views::enumerate(queue_families)) {
			if (family.queueFlags & flags) {
				if (first_applicable < 0)
					first_applicable = i;
				
				if (not queue_allocation.contains(i))
					return i;
			}
		}

		// Otherwise, prefer the first applicable
		return first_applicable;
	};

	const auto priority = 1.0f;
	auto queue_create_infos = std::vector <vk::DeviceQueueCreateInfo> ();
	queue_create_infos.reserve(options.queues.size());

	for (auto &[key, flags] : options.queues) {
		auto idx = queue_family_index(flags);
		if (idx == -1) {
			std::println(std::cerr, "failed to find a suitable queue for '{}'", key);
			continue;
		}

		queue_create_infos.emplace_back(
			vk::DeviceQueueCreateInfo()
				.setQueueFamilyIndex(idx)
				.setQueuePriorities(priority)
				.setQueueCount(1)
		);

		device.queues.emplace(key, Queue(nullptr, idx, 0));
	}

	auto device_info = vk::DeviceCreateInfo()
		.setQueueCreateInfos(queue_create_infos)
		.setPEnabledExtensionNames(options.extensions)
		.setPNext(options.pNext);

	device.logical = device.physical.createDevice(device_info);

	dld.init(device.logical);
	device.loader = dld;

	// Populate the queue handles
	for (auto &[_, queue] : device.queues)
		Tas <vk::Queue &> (queue) = device.logical.getQueue(queue.family_index, 0);

	return device;
}

} // namespace rcgp
