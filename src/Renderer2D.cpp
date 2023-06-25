#include "VividX/Renderer2D.h"
#include "VividX/Globals.h"
#include "VividX/VKContext.h"
#include "glm/ext/vector_float4.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include "vividx.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <SDL_video.h>
#include <cassert>
#include <memory>
#include <utility>

using namespace vividX;

Renderer2D::Renderer2D(SDL_Window *window)

{

  vk::SurfaceFormatKHR swapchainSurfaceFormat =
      vividX::chooseFormat(g_vkContext->physicalDevice, g_vkContext->surface,
                           g_vkContext->queueFamilyIndices["Graphics"].value());

  vk::PresentModeKHR presentMode = vividX::choosePresentMode(
      g_vkContext->physicalDevice, g_vkContext->surface);

  vk::Extent2D swapChainExtent = vividX::chooseExtent(
      g_vkContext->physicalDevice, g_vkContext->surface, window);

  m_SwapChain = std::make_unique<vividX::SwapChain>(
      Vector2ui{swapChainExtent.width, swapChainExtent.height},
      vk::PresentModeKHR::eMailbox);

  m_Renderpass = std::make_unique<vividX::RenderPass>(swapchainSurfaceFormat);

  m_SwapChain->createFrameBuffers(m_Renderpass.get());

  vertices = {{{50.0f, 0.f}, {1.0f, 0.0f, 0.0f}},
              {{100.0f, 100.0f}, {0.0f, 1.0f, 0.0f}},
              {{0.f, 100.5f}, {0.0f, 0.0f, 1.0f}}};

  m_vertexBuffer = std::make_unique<vividX::VertexBuffer>(sizeof(vertices[0]) *
                                                          vertices.size());

  m_vertexBuffer->update(vertices);

  vk::CommandPoolCreateInfo poolInfo{};
  poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  poolInfo.queueFamilyIndex =
      g_vkContext->queueFamilyIndices["Graphics"].value();

  assert(g_vkContext->device.createCommandPool(
             &poolInfo, nullptr, &m_commandpool) == vk::Result::eSuccess);

  vk::CommandBufferAllocateInfo info{};
  info.commandPool = m_commandpool;
  info.level = vk::CommandBufferLevel::ePrimary;
  info.commandBufferCount = 1;

  assert(g_vkContext->device.allocateCommandBuffers(&info, &m_commandBuffer) ==
         vk::Result::eSuccess);

  m_PipelineLayout = std::make_unique<PipelineLayout>();

  vk::PushConstantRange pushConstantRange{

  };
  pushConstantRange.offset = 0;

  pushConstantRange.size = sizeof(MeshPushConstants);
  pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;

  m_PipelineLayout->addPushConstantRange(pushConstantRange);

  m_graphicsPipeline = std::make_unique<GraphicsPipeline>(
      "shaders/frag.spv", "shaders/vert.spv", m_Renderpass->get(),
      m_PipelineLayout->get(),
      Vector2{(float)swapChainExtent.width, (float)swapChainExtent.height});

  vk::SemaphoreCreateInfo sephInfo{};
  vk::FenceCreateInfo fenceInfo{};
  fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

  vk::resultCheck(g_vkContext->device.createSemaphore(
                      &sephInfo, nullptr, &g_vkContext->ImageAvailable),
                  "failed to create imageSemaphore");
  vk::resultCheck(g_vkContext->device.createSemaphore(
                      &sephInfo, nullptr, &g_vkContext->RenderFinished),
                  "Failed to create renderfinished semaphore");

  vk::resultCheck(g_vkContext->device.createFence(&fenceInfo, nullptr,
                                                  &g_vkContext->inFlightFence),
                  "failed to create inFlight fence");

  camera = std::make_unique<Camera2D>(100, 100);
}

void Renderer2D::recordCommandBuffer(uint32_t imageIndex) {
  vk::CommandBufferBeginInfo beginInfo{};
  vk::ClearValue clearValue{};

  clearValue.color = {m_clearColour.x, m_clearColour.y, m_clearColour.z, 1.0f};

  vk::resultCheck(m_commandBuffer.begin(&beginInfo), "");

  vk::RenderPassBeginInfo renderPassBeginInfo{};

  renderPassBeginInfo.renderPass = m_Renderpass->get();
  renderPassBeginInfo.framebuffer = m_SwapChain->getFrameBuffer(imageIndex);

  renderPassBeginInfo.renderArea.offset = vk::Offset2D(0, 0);
  renderPassBeginInfo.renderArea.extent = m_SwapChain->getExtent();

  renderPassBeginInfo.clearValueCount = 1;
  renderPassBeginInfo.pClearValues = &clearValue;

  m_commandBuffer.beginRenderPass(&renderPassBeginInfo,
                                  vk::SubpassContents::eInline);

  m_commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                               m_graphicsPipeline->get());

  // TODO pushConstants
  MeshPushConstants pushconstants;
  pushconstants.data = glm::vec4(1.0f);
  pushconstants.render_matrix = camera->getViewProjMatrix();
  m_commandBuffer.pushConstants(m_PipelineLayout->get(),
                                vk::ShaderStageFlagBits::eVertex, 0,
                                sizeof(pushconstants), &pushconstants);

  vk::DeviceSize offsets = {0};

  m_commandBuffer.bindVertexBuffers(0, 1, &m_vertexBuffer->getBufferRef(),
                                    &offsets);

  m_commandBuffer.draw(vertices.size(), 1, 0, 0);
  ImGui::ShowDebugLogWindow();
  ImGui::Render();
  auto draw_data = ImGui::GetDrawData();

  const bool is_minimized =
      (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
  if (!is_minimized) {
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_commandBuffer);
  }

  m_commandBuffer.endRenderPass();
  m_commandBuffer.end();
}

void Renderer2D::beginFrame() {

  auto res = g_vkContext->device.waitForFences(1, &g_vkContext->inFlightFence,
                                               vk::Bool32(true), UINT64_MAX);
  vk::resultCheck(res, "waitForfences failed");

  res = g_vkContext->device.resetFences(1, &g_vkContext->inFlightFence);
  vk::resultCheck(res, "resetFences failed");
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
}

void Renderer2D::drawFrame() {

  vk::AcquireNextImageInfoKHR info;
  info.swapchain = m_SwapChain->get();
  info.semaphore = g_vkContext->ImageAvailable;
  info.timeout = UINT64_MAX;

  auto imageIndex = g_vkContext->device.acquireNextImageKHR(
      m_SwapChain->get(), UINT64_MAX, g_vkContext->ImageAvailable);

  vk::resultCheck(imageIndex.result, "");

  m_commandBuffer.reset();
  recordCommandBuffer(imageIndex.value);

  vk::SubmitInfo submitInfo{};

  vk::Semaphore waitSemaphores[] = {g_vkContext->ImageAvailable};

  vk::PipelineStageFlags waitStages[] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &m_commandBuffer;

  vk::Semaphore signalSephamores[] = {g_vkContext->RenderFinished};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSephamores;

  auto res = g_vkContext->graphicsQueue.submit(1, &submitInfo,
                                               g_vkContext->inFlightFence);
  assert(res == vk::Result::eSuccess);

  vk::SubpassDependency dep{};
  dep.srcSubpass = VK_SUBPASS_EXTERNAL;
  dep.dstSubpass = 0;

  dep.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

  dep.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dep.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

  vk::PresentInfoKHR presentInfo{};

  vk::SwapchainKHR swapChains[] = {m_SwapChain->get()};

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = &g_vkContext->RenderFinished;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex.value;
  presentInfo.pResults = nullptr;
  res = g_vkContext->presentQueue.presentKHR(&presentInfo);
  assert(res == vk::Result::eSuccess);
}

void Renderer2D::endFrame() {}
