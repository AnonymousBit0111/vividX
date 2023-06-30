#pragma once
#include <SDL.h>
#include <map>
#include <memory>

#include <vulkan/vulkan.hpp>

namespace vividX {

struct VKContext {
  vk::Instance instance;
  vk::Device device;
  vk::SurfaceKHR surface;
  vk::PhysicalDevice physicalDevice;
  vk::Queue graphicsQueue;
  vk::Queue presentQueue;
  vk::Fence inFlightFence;
  vk::Semaphore ImageAvailable;
  vk::Semaphore RenderFinished;
  std::map<std::string, std::optional<uint32_t>> queueFamilyIndices;

  bool destroyed = false;

  void destroy() {
    if (!destroyed) {
      device.destroySemaphore(RenderFinished);
      device.destroySemaphore(ImageAvailable);
      device.destroyFence(inFlightFence);
      device.destroy();
      instance.destroySurfaceKHR(surface);
      instance.destroy();

      destroyed = true;
    }
  }

  // Destructor
  ~VKContext() {
    // Cleanup code for the Vulkan resources
    if (!destroyed) {
      device.destroySemaphore(RenderFinished);
      device.destroySemaphore(ImageAvailable);
      device.destroyFence(inFlightFence);
      device.destroy();
      instance.destroySurfaceKHR(surface);
      instance.destroy();

      destroyed = true;
    }
  }
};

} // namespace vividX