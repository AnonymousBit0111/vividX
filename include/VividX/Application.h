#pragma once

#include "VividX/GraphicsPipeline.h"
#include "VividX/PipelineLayout.h"
#include "VividX/RenderPass.h"
#include "VividX/Renderer2D.h"
#include "VividX/SwapChain.h"
#include "VividX/VertexBuffer.h"
#include "vividx.h"
#include "vulkan/vulkan_handles.hpp"
#include <SDL2/SDL.h>
#include <_types/_uint32_t.h>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "VividX/Camera2D.h"
namespace vividX {
class Application {
private:
  SDL_Window *window;
  std::vector<const char *> validationLayers;
  VkDebugUtilsMessengerEXT debugMessenger;


  std::vector<PosColourVertex> vertices;

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
  vk::DescriptorPool ImGuiDescriptorPool;

  std::unique_ptr<Renderer2D> renderer;

  void initSDL();
  void initVulkan();

  // TODO move this to the renderer
  void initImGui();

  void createSurface();
  void createInstance();
  void createDebugCallback();
  void pickPhysicalDevice();
  void createLogicalDevice();

  void drawFrame();

  void mainLoop();
  void cleanup();

public:
  void run();
  Application();

  ~Application() {}
};

} // namespace vividX