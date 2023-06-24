#include "VividX/SwapChain.h"
#include "vividx.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <_types/_uint32_t.h>

using namespace vividX;

SwapChain::SwapChain(vk::PhysicalDevice &physicalDevice, vk::Device *device,
                     Vector2ui size, vk::SurfaceKHR surface,
                     uint32_t graphicsQueueIndex,

                     vk::PresentModeKHR presentmode,
                     vk::SwapchainKHR oldSwapchain)
    : p_logicalDevice(device), m_imageSize(size) {
  vk::SwapchainCreateInfoKHR createInfo{};

  auto format =
      vividX::chooseFormat(physicalDevice, surface, graphicsQueueIndex);
  auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);

  m_extent.setHeight(size.y);
  m_extent.setWidth(size.x);
  createInfo.imageFormat = format.format;
  createInfo.imageColorSpace = format.colorSpace;
  createInfo.minImageCount = capabilities.minImageCount + 1;

  createInfo.imageExtent = vk::Extent2D{size.x, size.y};
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
  createInfo.presentMode = presentmode;
  createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  createInfo.preTransform =
      physicalDevice.getSurfaceCapabilitiesKHR(surface).currentTransform;

  createInfo.clipped = 1;
  createInfo.surface = surface;
  createInfo.oldSwapchain = oldSwapchain;

  vk::resultCheck(
      device->createSwapchainKHR(&createInfo, nullptr, &m_rawSwapChain), "");

  m_images = device->getSwapchainImagesKHR(m_rawSwapChain);

  m_imageViews.resize(m_images.size());

  int index = 0;
  for (auto &i : m_images) {
    vk::ImageViewCreateInfo info{};
    info.image = i;
    info.viewType = vk::ImageViewType::e2D;
    info.format = format.format;

    info.components.r = vk::ComponentSwizzle::eIdentity;
    info.components.g = vk::ComponentSwizzle::eIdentity;
    info.components.b = vk::ComponentSwizzle::eIdentity;
    info.components.a = vk::ComponentSwizzle::eIdentity;

    info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;

    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    vk::Result res =
        device->createImageView(&info, nullptr, &m_imageViews[index]);
    if (res != vk::Result::eSuccess) {
      assert(false);
    }
    index++;
  }
}

void SwapChain::createFrameBuffers(RenderPass *renderpass) {
  m_framebuffers.resize(m_imageViews.size());

  for (size_t i = 0; i < m_imageViews.size(); i++) {
    vk::ImageView attachments[] = {m_imageViews[i]};
    vk::FramebufferCreateInfo frameBufferInfo{};
    frameBufferInfo.renderPass = renderpass->get();
    frameBufferInfo.attachmentCount = 1;
    frameBufferInfo.pAttachments = attachments;
    frameBufferInfo.width = m_imageSize.x;
    frameBufferInfo.height = m_imageSize.y;
    frameBufferInfo.layers = 1;
    vk::resultCheck(p_logicalDevice->createFramebuffer(
                        &frameBufferInfo, nullptr, &m_framebuffers[i]),
                    "");
  }
}
SwapChain::~SwapChain() {

  p_logicalDevice->destroySwapchainKHR(m_rawSwapChain);
}
