#pragma once

#include "SDL2/SDL_stdinc.h"
#include "VividX/Camera2D.h"
#include "VividX/GraphicsPipeline.h"
#include "VividX/IndexBuffer.h"
#include "VividX/PipelineLayout.h"
#include "VividX/Quad.h"
#include "VividX/RenderPass.h"
#include "VividX/SwapChain.h"
#include "VividX/VKContext.h"
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
  std::vector<PosColourVertex> vertices;
  std::vector<uint32_t> indices;
  Vector3 m_clearColour{0, 0, 0};

  std::unique_ptr<vividX::RenderPass> m_Renderpass;
  std::unique_ptr<vividX::SwapChain> m_SwapChain;
  std::unique_ptr<vividX::VertexBuffer> m_vertexBuffer;
  std::unique_ptr<vividX::IndexBuffer> m_indexBuffer;
  std::unique_ptr<vividX::PipelineLayout> m_PipelineLayout;
  std::unique_ptr<vividX::GraphicsPipeline> m_graphicsPipeline;

  vk::CommandBuffer m_commandBuffer;
  vk::CommandPool m_commandpool;
  vk::DescriptorPool ImGuiDescriptorPool;

  std::unique_ptr<Camera2D> camera;

  SDL_Window *p_window;

  // const int maxVerts = 40000;
  const int MaxQuads = 440000;
  const int MaxVerts = MaxQuads * 4;
  const int MaxIndexCount = MaxQuads * 6;

  int vertexCount;

  void initImGui();

public:
  Renderer2D(SDL_Window *window);

  void recordCommandBuffer(uint32_t imageIndex);

  RenderPass &getRenderPass() const { return *m_Renderpass; }
  SwapChain &getSwapChain() const { return *m_SwapChain; }
  vk::CommandPool &getCommandPool() { return m_commandpool; }
  void beginFrame(Camera2D &cam);

  void addQuad(Quad &q) ;
  void drawFrame();
  void endFrame();

  ~Renderer2D() {}
};
} // namespace vividX