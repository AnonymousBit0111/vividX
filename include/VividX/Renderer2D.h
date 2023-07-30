#pragma once

#include "SDL2/SDL_stdinc.h"
#include "VividX/Batch.h"
#include "VividX/Camera2D.h"
#include "VividX/GraphicsPipeline.h"
#include "VividX/IndexBuffer.h"
#include "VividX/PipelineLayout.h"
#include "VividX/Quad.h"
#include "VividX/RenderPass.h"
#include "VividX/SwapChain.h"
#include "VividX/UniformBuffer.h"
#include "VividX/VKContext.h"
#include "VividX/VertexBuffer.h"
#include "vividx.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <SDL_video.h>
#include <_types/_uint32_t.h>
#include <malloc/_malloc.h>
#include <map>
#include <memory>
namespace vividX {

class Renderer2D {

private:
  Vector3 m_clearColour{0, 0, 0};

  std::unique_ptr<vividX::RenderPass> m_Renderpass;
  std::unique_ptr<vividX::SwapChain> m_SwapChain;
  std::unique_ptr<vividX::VertexBuffer> m_vertexBuffer;
  std::unique_ptr<vividX::IndexBuffer> m_indexBuffer;
  std::unique_ptr<vividX::PipelineLayout> m_PipelineLayout;
  std::unique_ptr<vividX::GraphicsPipeline> m_graphicsPipeline;
  std::unique_ptr<vividX::UniformBuffer> m_uniformBuffer;

  vk::DescriptorSet m_descSet;

  vk::CommandBuffer m_commandBuffer;
  vk::CommandPool m_commandpool;
  vk::DescriptorPool ImGuiDescriptorPool;
  UniformBufferObject UBO;

  uint32_t instanceCount;

  std::unique_ptr<Camera2D> camera;
  Batch Quadbatch;

  SDL_Window *p_window;

  const int MaxQuads = 4400000;
  const int MaxVerts = MaxQuads * 4;
  const int MaxIndexCount = MaxQuads * 6;

  void initImGui();

public:
  Renderer2D(SDL_Window *window);

  void recordCommandBuffer(uint32_t imageIndex);

  RenderPass &getRenderPass() const { return *m_Renderpass; }
  SwapChain &getSwapChain() const { return *m_SwapChain; }
  vk::CommandPool &getCommandPool() { return m_commandpool; }

  UniformBufferObject &getUBO() { return UBO; }

  void resetInstances() { instanceCount = 0; }

  void uploadInstances();

  void addQuad(glm::mat4 &model);

  void beginFrame(Camera2D &cam);

  void drawFrame();
  void endFrame();

  ~Renderer2D() { delete[] UBO.models; }
};
} // namespace vividX