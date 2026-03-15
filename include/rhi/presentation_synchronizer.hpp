#pragma once

#include <cstddef>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace rcgp {

struct Device;

class PresentationSynchronizer {
    size_t current_slot = 0;
    uint32_t current_image = 0;
public:
    std::vector <vk::Fence> fences;
    std::vector <vk::Semaphore> acquire_semaphores;
    std::vector <vk::Semaphore> render_semaphores;

    vk::Semaphore acquire_semaphore() const;
    vk::Fence fence() const;
    vk::Semaphore render_semaphore() const;

    static PresentationSynchronizer from(const Device &device, size_t count);

    friend struct Device;
};

} // namespace rcgp
