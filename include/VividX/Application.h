#pragma once

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
  vk::SurfaceKHR surface;
  vk::PhysicalDevice physicalDevice;
  vk::UniqueDevice device;
  vk::Queue graphicsQueue;
  vk::Queue presentQueue;
  vk::SurfaceFormatKHR swapChainSurfaceFormat;
  vk::Extent2D swapChainExtent;
  uint32_t swapChainImageCount;
  vk::SwapchainKHR swapChain;
  std::vector<vk::Image> swapChainImages;
  std::vector<vk::ImageView> swapChainImageViews;
  std::vector<vk::Framebuffer> swapChainFrameBuffers;
  vk::CommandPool commandPool;
  vk::CommandBuffer commandBuffer;

  vk::PresentModeKHR presentMode;

  std::map<std::string, std::optional<uint32_t>> queueFamilyIndices;

  vk::RenderPass renderPass;

  vk::PipelineLayout pipelineLayout;

  vk::Pipeline graphicsPipeline;

  vk::Semaphore imageAvailableSeph;
  vk::Semaphore renderFinishedSeph;
   vk::Fence inFlightfence;

  void initSDL();
  void initVulkan();
  void createSurface();
  void createInstance();
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
  void createCommandBuffer();
  void createSyncObjects();

  void recordCommandBuffer(uint32_t imageIndex);

  void drawFrame();

  void mainLoop();
  void cleanup();

public:
  void run();

  ~Application() {}
};

} // namespace vividX