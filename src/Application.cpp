#include "VividX/Application.h"
#include "SDL2/SDL_keycode.h"
#include "SDL2/SDL_stdinc.h"
#include "SDL2/SDL_video.h"
#include "VividX/Camera2D.h"
#include "VividX/Globals.h"
#include "VividX/GraphicsPipeline.h"
#include "VividX/PipelineLayout.h"
#include "VividX/Quad.h"
#include "VividX/RenderPass.h"
#include "VividX/Renderer2D.h"
#include "VividX/SwapChain.h"
#include "VividX/VKContext.h"
#include "VividX/VertexBuffer.h"
#include "VkBootstrap.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_vulkan.h"
#include "vividx.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <_types/_uint32_t.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan_macos.h>

#include <glm/gtc/matrix_transform.hpp>

#include "tracy/Tracy.hpp"
const int WIDTH = 800;
const int HEIGHT = 600;

using namespace vividX;

std::shared_ptr<vividX::VKContext> vividX::g_vkContext = nullptr;
Application::Application() : camera(800, 600, 0, 0) {
  TracyFunction;
  ZoneScoped;

  g_vkContext = std::make_shared<VKContext>();
}
void Application::run() {
  TracyFunction;
  ZoneScoped;

  initSDL();
  initVulkan();

  mainLoop();
  cleanup();
}

void Application::initSDL() {
  TracyFunction;
  ZoneScoped;
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    assert(false && "Failed to initialize SDL.");
  }

  window = SDL_CreateWindow("Vulkan SDL2 Window", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT,
                            SDL_WINDOW_VULKAN);

  if (!window) {
    assert(false && "Failed to create SDL window.");
  }
}

void Application::initVulkan() {
  TracyFunction;
  ZoneScoped;
  createInstance();

  createSurface();
  pickPhysicalDevice();

  createLogicalDevice();

  renderer = std::make_unique<Renderer2D>(window);

  renderer->resetInstances();
  for (int i = 0; i < 10000; i++) {
    for (int r = 0; r < 440; r++) {
      auto a = glm::translate(glm::mat4(1), glm::vec3(i * 100, r * 100, 0));

      renderer->addQuad(a);
    }
  }
  renderer->uploadInstances();
}

void Application::initImGui() {
  TracyFunction;
  ZoneScoped;

  ImGui::CreateContext();
  ImGuiDescriptorPool = g_vkContext->device.createDescriptorPool(poolInfo);

  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.Allocator = nullptr;
  initInfo.Instance = g_vkContext->instance;
  initInfo.ImageCount = renderer->getSwapChain().getImageCount();
  initInfo.MinImageCount = 2;
  initInfo.Queue = g_vkContext->graphicsQueue;

  initInfo.QueueFamily = g_vkContext->queueFamilyIndices["Graphics"].value();
  initInfo.Device = g_vkContext->device;
  initInfo.PhysicalDevice = g_vkContext->physicalDevice;
  initInfo.CheckVkResultFn = nullptr;
  initInfo.DescriptorPool = ImGuiDescriptorPool;
  initInfo.PipelineCache = {};

  ImGui_ImplSDL2_InitForVulkan(window);
  ImGui_ImplVulkan_Init(&initInfo, renderer->getRenderPass().get());

  vk::CommandBufferAllocateInfo allocateInfo = {
      renderer->getCommandPool(),       // Command pool
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

void Application::createInstance() {
  TracyFunction;
  ZoneScoped;
  vk::ApplicationInfo appInfo{};
  appInfo.pApplicationName = "Vulkan SDL2 Demo";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_2;

  vk::InstanceCreateInfo createInfo{};
  createInfo.pApplicationInfo = &appInfo;

  std::vector<vk::LayerProperties> availableLayers =
      vk::enumerateInstanceLayerProperties();

  std::vector<std::string> requestedLayers = {"VK_LAYER_KHRONOS_validation"};
  std::vector<const char *> receivedLayers;

  for (auto &i : requestedLayers) {
    for (auto &layer : availableLayers) {
      if (std::string(layer.layerName) == std::string(i)) {
        log("validation layer " + i + " is enabled and supported",
            Severity::INFO);
        receivedLayers.push_back(layer.layerName);
      }
    }
  }

  createInfo.enabledLayerCount = receivedLayers.size();
  createInfo.ppEnabledLayerNames = receivedLayers.data();
  validationLayers = receivedLayers;

  uint32_t extensionCount = 0;
  if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr) ||
      extensionCount == 0) {
    assert(false && "Failed to get the number of Vulkan instance extensions.");
  }

  createInfo.enabledExtensionCount = extensionCount;
  std::vector<const char *> extensions(extensionCount);

  if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount,
                                        extensions.data())) {
    assert(false && "Failed to get Vulkan instance extensions.");
  }

  createInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;

  extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  for (auto &reqExt : extensions) {

    for (auto &i : vk::enumerateInstanceExtensionProperties()) {
      if (i.extensionName == std::string(reqExt)) {
        log("instance extension: " + std::string(i.extensionName) +
                " is supported",
            Severity::INFO);
      }
    }
  }
  createInfo.enabledExtensionCount = extensions.size();

  createInfo.ppEnabledExtensionNames = extensions.data();

  for (auto &i : extensions) {
    log("enabled instance extension:" + std::string(i), Severity::INFO);
  }

  auto res = vk::createInstance(&createInfo, nullptr, &g_vkContext->instance);

  vk::resultCheck(res, "error:");

  validationLayers = receivedLayers;

  log("succesfully created instance", Severity::INFO);
}

void Application::createDebugCallback() {
  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = vividX::debugCallback;

  if (vividX::CreateDebugUtilsMessengerEXT(g_vkContext->instance, &createInfo,
                                           nullptr,
                                           &debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug messenger!");
  }
}

void Application::createSurface() {
  TracyFunction;
  ZoneScoped;
  VkSurfaceKHR Csurf;
  if (!SDL_Vulkan_CreateSurface(window, g_vkContext->instance, &Csurf)) {
    assert(false && "Failed to create Vulkan surface.");
  }
  g_vkContext->surface = Csurf;
}

void Application::mainLoop() {
  TracyFunction;
  ZoneScoped;
  SDL_Event event;
  while (true) {

    if (ImGui::IsKeyDown(ImGuiKey_UpArrow)) {
      camera.move({0, -50});
    }
    if (ImGui::IsKeyDown(ImGuiKey_LeftArrow)) {
      camera.move({-50, 0});
    }
    if (ImGui::IsKeyDown(ImGuiKey_DownArrow)) {
      camera.move({0, 50});
    }
    if (ImGui::IsKeyDown(ImGuiKey_RightArrow)) {
      camera.move({50, 0});
    }
    if (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        break;
      }

      ImGui_ImplSDL2_ProcessEvent(&event);
    }
    drawFrame();

    SDL_UpdateWindowSurface(window);
    FrameMark;
  }
  g_vkContext->graphicsQueue.waitIdle();
  g_vkContext->device.waitIdle();
}

void Application::pickPhysicalDevice() {
  TracyFunction;
  ZoneScoped;

  std::vector<vk::PhysicalDevice> devices =
      g_vkContext->instance.enumeratePhysicalDevices();

  for (auto &i : devices) {
    vk::PhysicalDeviceProperties props = i.getProperties();
    vk::PhysicalDeviceFeatures features = i.getFeatures();

    if (props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
      std::string message = "Using gpu: ";
      message.append((const char *)props.deviceName);
      vividX::log(message, Severity::INFO);

      g_vkContext->physicalDevice = i;
    }
  }
}

void Application::createLogicalDevice() {
  TracyFunction;
  ZoneScoped;
  std::vector<vk::QueueFamilyProperties> queueFamilies =
      g_vkContext->physicalDevice.getQueueFamilyProperties();

  uint32_t i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
      g_vkContext->queueFamilyIndices["Graphics"] = i;
    }
    bool presentSupport;
    presentSupport = g_vkContext->physicalDevice.getSurfaceSupportKHR(
        i, g_vkContext->surface);
    if (presentSupport) {
      g_vkContext->queueFamilyIndices["Present"] = i;
    }
    i++;
  }

  assert(g_vkContext->queueFamilyIndices.count("Graphics") > 0 &&
         "Failed to find required graphics queue.");

  vk::DeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.queueFamilyIndex =
      g_vkContext->queueFamilyIndices["Graphics"].value();
  queueCreateInfo.queueCount = 1;
  static float queuePriority = 1.0f;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  vk::PhysicalDeviceFeatures deviceFeatures{};

  std::vector<vk::ExtensionProperties> availableExtensions =
      g_vkContext->physicalDevice.enumerateDeviceExtensionProperties();

  std::vector<const char *> enabledExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset"};

  for (auto &req : enabledExtensions) {
    bool extFound = false;
    for (auto &i :
         g_vkContext->physicalDevice.enumerateDeviceExtensionProperties()) {

      if (std::string(i.extensionName) == std::string(req)) {
        log("device extension " + std::string(i.extensionName) +
                " is enabled and supported",
            Severity::INFO);
        extFound = true;
        break;
      }
    }
    if (!extFound) {
      log("device extension " + std::string(req) + " not found",
          Severity::WARNING);
    }
  }

  vk::DeviceCreateInfo createInfo{};
  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(enabledExtensions.size());
  createInfo.ppEnabledExtensionNames = enabledExtensions.data();
  createInfo.queueCreateInfoCount = 1;
  createInfo.pQueueCreateInfos = &queueCreateInfo;
  createInfo.pEnabledFeatures = &deviceFeatures;

  g_vkContext->device = g_vkContext->physicalDevice.createDevice(createInfo);
  log("succesfully created logical device", Severity::INFO);

  g_vkContext->graphicsQueue = g_vkContext->device.getQueue(
      g_vkContext->queueFamilyIndices["Graphics"].value(), 0);
  g_vkContext->presentQueue = g_vkContext->device.getQueue(
      g_vkContext->queueFamilyIndices["Present"].value(), 0);
}

void Application::drawFrame() {
  TracyFunction;
  ZoneScoped;

  renderer->beginFrame(camera);

  renderer->endFrame();

  ImGui::ShowMetricsWindow();

  renderer->drawFrame();
}
void Application::cleanup() {
  TracyFunction;
  ZoneScoped;

  ImGui_ImplVulkan_Shutdown();

  SDL_DestroyWindow(window);

  SDL_Quit();
}
