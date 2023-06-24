#include "VividX/Renderer2D.h"
#include "glm/ext/vector_float4.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include "vividx.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <cassert>
#include <memory>
#include <utility>

using namespace vividX;

Renderer2D::Renderer2D(
    vk::Instance instance, std::unique_ptr<vk::Device> device,
    vk::SurfaceKHR surface, vk::PhysicalDevice physicalDevice,
    std::map<std::string, std::optional<uint32_t>> queueFamilyIndices,
    SDL_Window *window, vk::Queue graphicsQueue, vk::Queue presentQueue)

    : m_instance(instance), m_device(std::move(device)), m_surface(surface),
      m_physicalDevice(physicalDevice),
      m_queueFamilyIndices(queueFamilyIndices), m_graphicsQueue(graphicsQueue),
      m_presentQueue(presentQueue) {

  vk::SurfaceFormatKHR swapchainSurfaceFormat = vividX::chooseFormat(
      physicalDevice, surface, m_queueFamilyIndices["Graphics"].value());

  vk::PresentModeKHR presentMode =
      vividX::choosePresentMode(physicalDevice, surface);

  vk::Extent2D swapChainExtent =
      vividX::chooseExtent(physicalDevice, surface, window);

  m_SwapChain = std::make_unique<vividX::SwapChain>(
      physicalDevice, m_device.get(),
      Vector2ui{swapChainExtent.width, swapChainExtent.height}, surface,
      queueFamilyIndices["Graphics"].value(), vk::PresentModeKHR::eMailbox);

  m_Renderpass = std::make_unique<vividX::RenderPass>(m_device.get(),
                                                      swapchainSurfaceFormat);

  m_SwapChain->createFrameBuffers(m_Renderpass.get());

  vertices = {{{50.0f, 0.f}, {1.0f, 0.0f, 0.0f}},
              {{100.0f, 100.0f}, {0.0f, 1.0f, 0.0f}},
              {{0.f, 100.5f}, {0.0f, 0.0f, 1.0f}}};

  m_vertexBuffer = std::make_unique<vividX::VertexBuffer>(
      m_device.get(), sizeof(vertices[0]) * vertices.size(), physicalDevice);

  m_vertexBuffer->update(vertices);

  vk::CommandPoolCreateInfo poolInfo{};
  poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  poolInfo.queueFamilyIndex = queueFamilyIndices["Graphics"].value();

  assert(m_device->createCommandPool(&poolInfo, nullptr, &m_commandpool) ==
         vk::Result::eSuccess);

  vk::CommandBufferAllocateInfo info{};
  info.commandPool = m_commandpool;
  info.level = vk::CommandBufferLevel::ePrimary;
  info.commandBufferCount = 1;

  assert(m_device->allocateCommandBuffers(&info, &m_commandBuffer) ==
         vk::Result::eSuccess);

  m_PipelineLayout = std::make_unique<PipelineLayout>(m_device.get());

  vk::PushConstantRange pushConstantRange{

  };
  pushConstantRange.offset = 0;

  pushConstantRange.size = sizeof(MeshPushConstants);
  pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;

  m_PipelineLayout->addPushConstantRange(pushConstantRange);

  m_graphicsPipeline = std::make_unique<GraphicsPipeline>(
      "shaders/frag.spv", "shaders/vert.spv", m_device.get(),
      m_Renderpass->get(), m_PipelineLayout->get(),
      Vector2{(float)swapChainExtent.width, (float)swapChainExtent.height});

  vk::SemaphoreCreateInfo sephInfo{};
  vk::FenceCreateInfo fenceInfo{};
  fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

  vk::resultCheck(
      m_device->createSemaphore(&sephInfo, nullptr, &m_ImageAvailable),
      "failed to create imageSemaphore");
  vk::resultCheck(
      m_device->createSemaphore(&sephInfo, nullptr, &m_RenderFinished),
      "Failed to create renderfinished semaphore");

  vk::resultCheck(m_device->createFence(&fenceInfo, nullptr, &m_inFlightFence),
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

  auto res = m_device->waitForFences(1, &m_inFlightFence, vk::Bool32(true),
                                     UINT64_MAX);
  vk::resultCheck(res, "waitForfences failed");

  res = m_device->resetFences(1, &m_inFlightFence);
  vk::resultCheck(res, "resetFences failed");
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
}

void Renderer2D::drawFrame() {

  vk::AcquireNextImageInfoKHR info;
  info.swapchain = m_SwapChain->get();
  info.semaphore = m_ImageAvailable;
  info.timeout = UINT64_MAX;

  auto imageIndex = m_device->acquireNextImageKHR(m_SwapChain->get(),
                                                  UINT64_MAX, m_ImageAvailable);

  vk::resultCheck(imageIndex.result, "");

  m_commandBuffer.reset();
  recordCommandBuffer(imageIndex.value);

  vk::SubmitInfo submitInfo{};

  vk::Semaphore waitSemaphores[] = {m_ImageAvailable};

  vk::PipelineStageFlags waitStages[] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &m_commandBuffer;

  vk::Semaphore signalSephamores[] = {m_RenderFinished};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSephamores;

  auto res = m_graphicsQueue.submit(1, &submitInfo, m_inFlightFence);
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
  presentInfo.pWaitSemaphores = &m_RenderFinished;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex.value;
  presentInfo.pResults = nullptr;
  res = m_presentQueue.presentKHR(&presentInfo);
  assert(res == vk::Result::eSuccess);
}

void Renderer2D::endFrame() {}
