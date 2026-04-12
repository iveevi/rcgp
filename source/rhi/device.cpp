#include <ranges>
#include <algorithm>
#include <limits>
#include <print>
#include <set>

#include <GLFW/glfw3.h>

#include "meta/command_stream.hpp"
#include "meta/commands.hpp"
#include "rhi/buffer.hpp"
#include "rhi/xlas.hpp"
#include "rhi/command_buffer.hpp"
#include "rhi/command_pool.hpp"
#include "rhi/descriptor_pool.hpp"
#include "rhi/device.hpp"
#include "rhi/presentation_synchronizer.hpp"
#include "rhi/timestamp_pool.hpp"
#include "rhi/window.hpp"
#include "util/cti.hpp"
#include "util/error.hpp"

namespace rcgp {

vk::DebugUtilsObjectNameInfoEXT name_info(const vk::CommandBuffer &cmd)
{
	auto info = vk::DebugUtilsObjectNameInfoEXT();
	info.setObjectHandle(*(reinterpret_cast <const uint64_t *> (&cmd)));
	info.setObjectType(vk::ObjectType::eCommandBuffer);
	return info;
}

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

	fatal("no compatible memory type for buffer");
}

auto Device::get_address(const Buffer &buffer) const
	-> vk::DeviceAddress
{
	auto info = vk::BufferDeviceAddressInfo()
		.setBuffer(buffer.handle);
	return logical.getBufferAddress(info);
}

auto Device::get_address(const AccelerationStructure &as) const
	-> vk::DeviceAddress
{
	return logical.getAccelerationStructureAddressKHR(as.handle, loader);
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

auto Device::new_command_modules(const CommandPool &cpool, size_t count) const
	-> std::vector <Commands <true>>
{
	auto buffers = new_command_buffers(cpool, count);
	auto modules = std::vector <Commands <true>> {};
	modules.reserve(buffers.size());
	for (auto &b : buffers)
		modules.emplace_back(b);
	return modules;
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
	
std::optional <uint32_t> Device::acquire_next_frame(
	Window &window,
	PresentationSynchronizer &sync,
	uint64_t timeout
) const
{
	sync.current_slot = (sync.current_slot + 1) % sync.fences.size();

	auto wait = logical.waitForFences(sync.fences[sync.current_slot], true, timeout);
	if (wait != vk::Result::eSuccess && wait != vk::Result::eTimeout)
		fatal("waitForFences failed ({})", vk::to_string(wait));

	uint32_t image_index = 0;
	auto acq_result = static_cast <vk::Result> (vkAcquireNextImageKHR(
		logical,
		window.swapchain,
		timeout,
		sync.acquire_semaphores[sync.current_slot],
		nullptr,
		&image_index
	));

	if (acq_result == vk::Result::eErrorOutOfDateKHR or acq_result == vk::Result::eSuboptimalKHR) {
		window.is_swapchain_out_of_date = true;
		return std::nullopt;
	}

	if (acq_result != vk::Result::eSuccess)
		fatal("acquireNextImageKHR failed ({})", vk::to_string(acq_result));

	logical.resetFences(sync.fences[sync.current_slot]);
	sync.current_image = image_index;
	return image_index;
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

	auto new_images = std::vector <ColorTargetImage> ();
	new_images.reserve(swapchain_images.size());
	for (auto &handle : swapchain_images) {
		ColorTargetImage image;
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

	for (auto &image : window.images)
		image.destroy();

	if (window.swapchain)
		logical.destroySwapchainKHR(window.swapchain);

	window.swapchain = new_swapchain;
	window.images = std::move(new_images);
	window.swapchain_extent = new_extent;
	window.is_swapchain_out_of_date = false;
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
