

#include "VividX/RenderPass.h"
#include "VividX/Globals.h"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <memory>
#include "vividx.h"
using namespace vividX;
RenderPass::RenderPass(vk::SurfaceFormatKHR format) {
  TracyFunction;
  ZoneScoped;
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

  auto res =
      g_vkContext->device.createRenderPass(&Info, nullptr, &m_renderPass);

  vk::resultCheck(res, "error, renderPass creation failed");
}