

#include "VividX/RenderPass.h"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <memory>
using namespace vividX;
RenderPass::RenderPass(vk::Device *dev, vk::SurfaceFormatKHR format)
:p_device(dev) {

  vk::RenderPassCreateInfo Info{};
  vk::AttachmentDescription colourAttachment{};
  colourAttachment.format = format.format;
  colourAttachment.samples = vk::SampleCountFlagBits::e1;

  colourAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  colourAttachment.storeOp = vk::AttachmentStoreOp::eStore;

  colourAttachment.initialLayout = vk::ImageLayout::eUndefined;
  colourAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
  vk::AttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;
  

  vk::SubpassDescription subpass{};
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  Info.attachmentCount = 1;
  Info.pAttachments = &colourAttachment;
  Info.subpassCount = 1;
  Info.pSubpasses = &subpass;

  vk::RenderPass temp;

  auto res = dev->createRenderPass(&Info, nullptr, &m_renderPass);

  vk::resultCheck(res, "error, renderPass creation failed");
}