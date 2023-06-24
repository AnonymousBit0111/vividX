#pragma once

#include "VividX/Camera2D.h"
#include "VividX/GraphicsPipeline.h"
#include "VividX/PipelineLayout.h"
#include "VividX/RenderPass.h"
#include "VividX/SwapChain.h"
#include "VividX/VertexBuffer.h"
#include "vividx.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <SDL_video.h>
#include <_types/_uint32_t.h>
#include <map>
#include <memory>
namespace vividX {

class Renderer2D {

private:
  vk::Instance m_instance;
  std::unique_ptr<vk::Device> m_device;
  vk::SurfaceKHR m_surface;
  vk::PhysicalDevice m_physicalDevice;
  vk::CommandPool m_commandpool;
  vk::CommandBuffer m_commandBuffer;
  vk::Queue m_graphicsQueue;
  vk::Queue m_presentQueue;
  vk::Fence m_inFlightFence;
  vk::Semaphore m_ImageAvailable;
  vk::Semaphore m_RenderFinished;

  std::map<std::string, std::optional<uint32_t>> m_queueFamilyIndices;
  std::vector<PosColourVertex> vertices;
  Vector3 m_clearColour{0, 0, 0};

  std::unique_ptr<vividX::RenderPass> m_Renderpass;
  std::unique_ptr<vividX::SwapChain> m_SwapChain;
  std::unique_ptr<vividX::VertexBuffer> m_vertexBuffer;
  std::unique_ptr<vividX::PipelineLayout> m_PipelineLayout;
  std::unique_ptr<vividX::GraphicsPipeline> m_graphicsPipeline;

  std::unique_ptr<Camera2D> camera;

public:
  Renderer2D(vk::Instance instance, std::unique_ptr<vk::Device> device,
             vk::SurfaceKHR surface, vk::PhysicalDevice physicalDevice,
             std::map<std::string, std::optional<uint32_t>> queueFamilyIndices,
             SDL_Window *window, vk::Queue graphicsQueue,
             vk::Queue presentQueue);

  void recordCommandBuffer(uint32_t imageIndex);

  vk::Device &getDevice() const { return *m_device; }
  RenderPass &getRenderPass() const { return *m_Renderpass; }
  SwapChain &getSwapChain() const { return *m_SwapChain; }
  vk::CommandPool &getCommandPool() { return m_commandpool; }
  void beginFrame();
  void drawFrame();
  void endFrame();

  ~Renderer2D() {}
};
} // namespace vividX