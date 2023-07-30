#include "VividX/Renderer2D.h"
#include "VividX/Batch.h"
#include "VividX/Camera2D.h"
#include "VividX/Globals.h"
#include "VividX/IndexBuffer.h"
#include "VividX/Quad.h"
#include "VividX/UniformBuffer.h"
#include "VividX/VKContext.h"
#include "VividX/VertexBuffer.h"
#include "glm/ext/vector_float2.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/fwd.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include "tracy/TracyC.h"
#include "vividx.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <SDL_video.h>
#include <_types/_uint32_t.h>
#include <cassert>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

using namespace vividX;

void Renderer2D::initImGui() {

  TracyFunction;
  ZoneScoped;
  ImGui::CreateContext();

  std::vector<vk::DescriptorPoolSize> poolSizes = {
      {vk::DescriptorType::eSampler, 1000},
      {vk::DescriptorType::eCombinedImageSampler, 1000},
      {vk::DescriptorType::eSampledImage, 1000},
      {vk::DescriptorType::eStorageImage, 1000},
      {vk::DescriptorType::eUniformTexelBuffer, 1000},
      {vk::DescriptorType::eStorageTexelBuffer, 1000},
      {vk::DescriptorType::eUniformBuffer, 1000},
      {vk::DescriptorType::eStorageBuffer, 1000},
      {vk::DescriptorType::eUniformBufferDynamic, 1000},
      {vk::DescriptorType::eStorageBufferDynamic, 1000},
      {vk::DescriptorType::eInputAttachment, 1000}};

  // Create the descriptor pool
  vk::DescriptorPoolCreateInfo poolInfo = {
      {},                                      // Flags
      1000,                                    // Max sets
      static_cast<uint32_t>(poolSizes.size()), // Pool size count
      poolSizes.data()                         // Pool sizes
  };
  ImGuiDescriptorPool = g_vkContext->device.createDescriptorPool(poolInfo);

  vk::DescriptorSetAllocateInfo allocInfo;

  allocInfo.descriptorPool = ImGuiDescriptorPool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = m_PipelineLayout->getLayouts();

  assert(g_vkContext->device.allocateDescriptorSets(&allocInfo, &m_descSet) ==
         vk::Result::eSuccess);

  vk::DescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = m_uniformBuffer->getBuffer();
  bufferInfo.offset = 0;
  bufferInfo.range = sizeof(glm::mat4) * MaxQuads;

  vk::WriteDescriptorSet descWrite{};
  descWrite.dstSet = m_descSet;
  descWrite.dstBinding = 0;
  descWrite.dstArrayElement = 0;
  descWrite.descriptorType = vk::DescriptorType::eUniformBuffer;
  descWrite.descriptorCount = 1;
  descWrite.pBufferInfo = &bufferInfo;

  g_vkContext->device.updateDescriptorSets(1, &descWrite, 0, nullptr);

  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.Allocator = nullptr;
  initInfo.Instance = g_vkContext->instance;
  initInfo.ImageCount = m_SwapChain->getImageCount();
  initInfo.MinImageCount = 2;
  initInfo.Queue = g_vkContext->graphicsQueue;

  initInfo.QueueFamily = g_vkContext->queueFamilyIndices["Graphics"].value();
  initInfo.Device = g_vkContext->device;
  initInfo.PhysicalDevice = g_vkContext->physicalDevice;
  initInfo.CheckVkResultFn = nullptr;
  initInfo.DescriptorPool = ImGuiDescriptorPool;
  initInfo.PipelineCache = {};

  ImGui_ImplSDL2_InitForVulkan(p_window);
  ImGui_ImplVulkan_Init(&initInfo, m_Renderpass->get());

  vk::CommandBufferAllocateInfo allocateInfo = {
      m_commandpool,                    // Command pool
      vk::CommandBufferLevel::ePrimary, // Command buffer level
      1                                 // Number of command buffers to allocate
  };
  vk::CommandBuffer tempBuffer;

  vk::resultCheck(
      g_vkContext->device.allocateCommandBuffers(&allocateInfo, &tempBuffer),
      "");

  vk::CommandBufferBeginInfo beginInfo = {
      vk::CommandBufferUsageFlagBits::eOneTimeSubmit, // Flags
      nullptr // Pointer to a VkCommandBufferInheritanceInfo struct
  };

  ImGui::StyleColorsDark();
  tempBuffer.begin(beginInfo);

  ImGui_ImplVulkan_CreateFontsTexture(tempBuffer);

  tempBuffer.end();

  vk::SubmitInfo submitInfo = {
      {}, // Wait semaphores
      {}, // Wait stages
      {}, // Command buffers
      {}, // Signal semaphores
  };
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &tempBuffer;

  g_vkContext->graphicsQueue.submit(submitInfo);
  g_vkContext->graphicsQueue.waitIdle();

  ImGui_ImplVulkan_DestroyFontUploadObjects();
}
static Quad q(glm::vec2(1 * 100, 1 * 100), glm::vec2(100, 100));

Renderer2D::Renderer2D(SDL_Window *window) : p_window(window) {
  TracyFunction;
  ZoneScoped;

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

  m_vertexBuffer = std::make_unique<vividX::VertexBuffer>(
      sizeof(Quadbatch.vertices[0]) * MaxVerts);
  m_indexBuffer = std::make_unique<IndexBuffer>(MaxIndexCount);

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
  auto binding = UniformBuffer::getDefaultBinding();

  vk::DescriptorSetLayoutCreateInfo layoutCreateInfo{};
  layoutCreateInfo.bindingCount = 1;
  layoutCreateInfo.pBindings = &binding;

  m_PipelineLayout->addDescriptorSetLayout(
      g_vkContext->device.createDescriptorSetLayout(layoutCreateInfo));

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

  m_uniformBuffer =
      std::make_unique<UniformBuffer>(sizeof(glm::mat4) * MaxQuads);

  m_vertexBuffer->update(q.vertices);

  UBO.models = new glm::mat4[MaxQuads];
  std::vector<uint32_t> indices;
  for (auto &i : Quad::indices) {
    indices.push_back(i);
  }

  m_indexBuffer->update(indices);

  initImGui();
}

void Renderer2D::recordCommandBuffer(uint32_t imageIndex) {
  TracyFunction;
  ZoneScoped;

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

  MeshPushConstants pushconstants;
  pushconstants.data = glm::vec4(1.0f);
  pushconstants.render_matrix = camera->getViewProjMatrix();
  m_commandBuffer.pushConstants(m_PipelineLayout->get(),
                                vk::ShaderStageFlagBits::eVertex, 0,
                                sizeof(pushconstants), &pushconstants);

  vk::DeviceSize offsets = {0};

  m_commandBuffer.bindVertexBuffers(0, 1, &m_vertexBuffer->getBufferRef(),
                                    &offsets);
  m_commandBuffer.bindIndexBuffer(m_indexBuffer->get(), 0,
                                  vk::IndexType::eUint32);

  m_commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                     m_PipelineLayout->get(), 0, 1, &m_descSet,
                                     0, nullptr);
  m_commandBuffer.drawIndexed(6, instanceCount, 0, 0, 0);

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

void Renderer2D::uploadInstances() {
  m_uniformBuffer->update(UBO, instanceCount);
}

void Renderer2D::addQuad(glm::mat4 &model) {

  UBO.models[instanceCount] = model;
  instanceCount++;
}

void Renderer2D::beginFrame(Camera2D &cam) {
  TracyFunction;
  ZoneScoped;

  auto res = g_vkContext->device.waitForFences(1, &g_vkContext->inFlightFence,
                                               vk::Bool32(true), UINT64_MAX);
  vk::resultCheck(res, "waitForfences failed");

  res = g_vkContext->device.resetFences(1, &g_vkContext->inFlightFence);
  vk::resultCheck(res, "resetFences failed");
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  *camera = cam;
  Quadbatch.vertexCount = 0;
}

void Renderer2D::drawFrame() {
  TracyFunction;
  ZoneScoped;

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

void Renderer2D::endFrame() {
  TracyFunction;
  ZoneScoped;
}
