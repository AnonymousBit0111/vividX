#pragma once

#include "VividX/RenderPass.h"
#include "vividx.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <_types/_uint32_t.h>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>
namespace vividX {

class SwapChain {

private:
  vk::SwapchainKHR m_rawSwapChain;
  vk::SurfaceFormatKHR m_format;

  vk::Device *p_logicalDevice; // all objects must have a shorter lifetime than
  // the logical device

  std::vector<vk::Image> m_images;
  std::vector<vk::ImageView> m_imageViews;
  std::vector<vk::Framebuffer> m_framebuffers;

  Vector2ui m_imageSize;
  vk::Extent2D m_extent;

public:
  SwapChain(vk::PhysicalDevice &physicalDevice, vk::Device *device,
            Vector2ui size, vk::SurfaceKHR surface, uint32_t graphicsQueueIndex,
            vk::PresentModeKHR presentmode = vk::PresentModeKHR::eFifo,
            vk::SwapchainKHR oldSwapchain = {});

  void createFrameBuffers(RenderPass *renderpass);

  vk::SwapchainKHR get() { return m_rawSwapChain; }

  inline vk::Framebuffer getFrameBuffer(uint32_t index) {
    return m_framebuffers[index];
  }
  uint32_t getImageCount() { return m_images.size(); }

  vk::Extent2D getExtent(){return m_extent;}


  ~SwapChain();
};
} // namespace vividX