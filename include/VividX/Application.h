#pragma once

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
namespace vividX {
class Application {
private:
  SDL_Window *window;
  vk::Instance instance;
  std::vector<const char *> validationLayers;
  VkDebugUtilsMessengerEXT debugMessenger;
  vk::SurfaceKHR surface;
  vk::PhysicalDevice physicalDevice;
  vk::Queue graphicsQueue;
  vk::Queue presentQueue;
  vk::SurfaceFormatKHR swapChainSurfaceFormat;
  vk::Extent2D swapChainExtent;
  uint32_t swapChainImageCount;
  vk::SwapchainKHR swapChain{};
  std::vector<vk::Image> swapChainImages;
  std::vector<vk::ImageView> swapChainImageViews;
  std::vector<vk::Framebuffer> swapChainFrameBuffers;
  vk::CommandPool commandPool;
  vk::CommandBuffer commandBuffer;
  vk::RenderPassCreateInfo renderPassInfo{};

  vk::PresentModeKHR presentMode;

  std::map<std::string, std::optional<uint32_t>> queueFamilyIndices;
  std::vector<PosColourVertex> vertices;
  vk::DeviceMemory vertexBufferMemory;
  vk::RenderPass renderPass;

  vk::PipelineLayout pipelineLayout;

  vk::Pipeline graphicsPipeline;

  vk::Semaphore imageAvailableSeph;
  vk::Semaphore renderFinishedSeph;
  vk::Fence inFlightfence;
  vk::Buffer vertexBuffer;
  std::unique_ptr<vk::Device> device;
  vk::DescriptorPool descPool;

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

  void initSDL();
  void initVulkan();
  void initImGui();
  void createSurface();
  void createInstance();
  void createDebugCallback();
  void pickPhysicalDevice();
  void createLogicalDevice();
  vk::SurfaceFormatKHR chooseFormat();
  vk::PresentModeKHR choosePresentMode();
  vk::Extent2D chooseExtent();
  void createSwapChain();
  void createRenderPass();
  void createGraphicsPipeline();

  void createFramebuffers();
  void createCommandPool();

  void createVertexBuffer();
  void createCommandBuffer();
  void createSyncObjects();

  void recordCommandBuffer(uint32_t imageIndex);

  void drawFrame();

  void mainLoop();
  void cleanup();
  static vk::VertexInputBindingDescription getBindingDescription();
  static std::array<vk::VertexInputAttributeDescription, 2> getAttribDesc();
  uint32_t findMemoryType(uint32_t typeFilter,
                          vk::MemoryPropertyFlags properties);

public:
  void run();

  ~Application() {}
};

} // namespace vividX