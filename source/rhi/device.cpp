#include "rhi/command_pool.hpp"
#include "rhi/descriptor_pool.hpp"
#include "rhi/device.hpp"
#include "rhi/window.hpp"

#include "util/logging.hpp"

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

auto Device::new_framebuffer(const vk::RenderPass &render_pass, const std::span <const vk::ImageView> &attachments, const vk::Extent2D &extent, uint32_t layers) const
	-> vk::Framebuffer
{
	auto fb_info = vk::FramebufferCreateInfo()
		.setRenderPass(render_pass)
		.setAttachments(attachments)
		.setWidth(extent.width)
		.setHeight(extent.height)
		.setLayers(layers);

	return logical.createFramebuffer(fb_info);
}

auto Device::new_command_buffers(const CommandPool &cpool, size_t count) const
	-> std::vector <vk::CommandBuffer>
{
	auto info = vk::CommandBufferAllocateInfo()
		.setCommandBufferCount(count)
		.setCommandPool(cpool);

	return logical.allocateCommandBuffers(info);
}
	
auto Device::new_descriptor_sets(const DescriptorPool &dpool, const vk::ArrayProxy <vk::DescriptorSetLayout> &dsls) const
	-> std::vector <vk::DescriptorSet>
{
	auto info = vk::DescriptorSetAllocateInfo()
		.setDescriptorPool(dpool)
		.setSetLayouts(dsls);

	return logical.allocateDescriptorSets(info);
}
	
void Device::wait_for_frame(const Frame &frame, uint64_t timeout) const
{
	auto result = logical.waitForFences(frame.fence, true, timeout);
	logical.resetFences(frame.fence);
}

bool Device::acquire_image_for_frame(Frame &frame, uint64_t timeout) const
{
	auto acq = logical.acquireNextImageKHR(
		frame.swapchain,
		timeout,
		frame.presented,
		nullptr
	);

	frame.image_index = acq.value;

	return !(acq.result == vk::Result::eErrorOutOfDateKHR || acq.result == vk::Result::eSuboptimalKHR);
}

Device Device::from(const Session &session, vk::detail::DispatchLoaderDynamic &dld, const Info &info)
{
	Device device;

	device.physical = session.handle.enumeratePhysicalDevices().front();
	device.properties = device.physical.getMemoryProperties();

	auto priority = 1.0f;

	auto device_queues_info = vk::DeviceQueueCreateInfo()
		.setQueueFamilyIndex(0)
		.setQueuePriorities(priority)
		.setQueueCount(1);

	auto device_info = vk::DeviceCreateInfo()
		.setQueueCreateInfos(device_queues_info)
		.setPEnabledExtensionNames(info.extensions);

	device.logical = device.physical.createDevice(device_info);

	dld.init(device.logical);

	return device;
}
