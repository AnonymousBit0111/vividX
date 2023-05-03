#pragma once

#include "vividx.h"
#include "vulkan/vulkan_handles.hpp"
#include <SDL2/SDL.h>
#include <_types/_uint32_t.h>
#include <cstdint>
#include <map>
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
  vk::UniqueDevice device;
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

  void initSDL();
  void initVulkan();
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