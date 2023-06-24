#include "VividX/Application.h"
#include "SDL2/SDL_stdinc.h"
#include "SDL2/SDL_video.h"
#include "VividX/GraphicsPipeline.h"
#include "VividX/PipelineLayout.h"
#include "VividX/RenderPass.h"
#include "VividX/Renderer2D.h"
#include "VividX/SwapChain.h"
#include "VividX/VertexBuffer.h"
#include "VkBootstrap.h"
#include "glm/ext/matrix_clip_space.hpp"
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
const int WIDTH = 800;
const int HEIGHT = 600;

using namespace vividX;
// TODO change sephamore to semaphore

Application::Application()

    : cam(Vector2{1600, 900}) {}
void Application::run() {

  vertices = {{{50.0f, 0.f}, {1.0f, 0.0f, 0.0f}},
              {{100.0f, 100.0f}, {0.0f, 1.0f, 0.0f}},
              {{0.f, 100.5f}, {0.0f, 0.0f, 1.0f}}

  };
  initSDL();
  initVulkan();

  initImGui();
  mainLoop();
  cleanup();
}

void Application::initSDL() {
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
  createInstance();

  createSurface();
  pickPhysicalDevice();

  createLogicalDevice();

  renderer = std::make_unique<Renderer2D>(instance, std::move(device), surface,
                                          physicalDevice, queueFamilyIndices,
                                          window, graphicsQueue, presentQueue);
}

void Application::initImGui() {

  ImGui::CreateContext();
  ImGuiDescriptorPool = renderer->getDevice().createDescriptorPool(poolInfo);

  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.Allocator = nullptr;
  initInfo.Instance = instance;
  initInfo.ImageCount = renderer->getSwapChain().getImageCount();
  initInfo.MinImageCount = 2;
  initInfo.Queue = graphicsQueue;

  initInfo.QueueFamily = queueFamilyIndices["Graphics"].value();
  initInfo.Device = renderer->getDevice();
  initInfo.PhysicalDevice = physicalDevice;
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
      renderer->getDevice().allocateCommandBuffers(&allocateInfo, &tempBuffer),
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

  graphicsQueue.submit(submitInfo);
  graphicsQueue.waitIdle();

  ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void Application::createInstance() {
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

  // extensions.push_back("VK_EXT_metal_surface");
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

  auto res = vk::createInstance(&createInfo, nullptr, &instance);

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

  if (vividX::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                           &debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug messenger!");
  }
}

void Application::createSurface() {
  VkSurfaceKHR Csurf;
  if (!SDL_Vulkan_CreateSurface(window, instance, &Csurf)) {
    assert(false && "Failed to create Vulkan surface.");
  }
  surface = Csurf;
}

void Application::mainLoop() {
  SDL_Event event;
  while (true) {
    if (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        break;
      }
      ImGui_ImplSDL2_ProcessEvent(&event);
    }
    drawFrame();

    SDL_UpdateWindowSurface(window);
  }
  renderer->getDevice().waitIdle();
}

void Application::pickPhysicalDevice() {

  std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();

  for (auto &i : devices) {
    vk::PhysicalDeviceProperties props = i.getProperties();
    vk::PhysicalDeviceFeatures features = i.getFeatures();

    if (props.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
      std::string message = "Using gpu: ";
      message.append((const char *)props.deviceName);
      vividX::log(message, Severity::INFO);

      // for (auto &i : i.enumerateDeviceExtensionProperties()) {
      //   log(i.extensionName, Severity::INFO);
      // }

      physicalDevice = i;
    }
  }
}

void Application::createLogicalDevice() {
  std::vector<vk::QueueFamilyProperties> queueFamilies =
      physicalDevice.getQueueFamilyProperties();

  uint32_t i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
      queueFamilyIndices["Graphics"] = i;
    }
    bool presentSupport;
    presentSupport = physicalDevice.getSurfaceSupportKHR(i, surface);
    if (presentSupport) {
      queueFamilyIndices["Present"] = i;
    }
    i++;
  }

  assert(queueFamilyIndices.count("Graphics") > 0 &&
         "Failed to find required graphics queue.");

  vk::DeviceQueueCreateInfo queueCreateInfo{};
  queueCreateInfo.queueFamilyIndex = queueFamilyIndices["Graphics"].value();
  queueCreateInfo.queueCount = 1;
  static float queuePriority = 1.0f;
  queueCreateInfo.pQueuePriorities = &queuePriority;

  vk::PhysicalDeviceFeatures deviceFeatures{};

  std::vector<vk::ExtensionProperties> availableExtensions =
      physicalDevice.enumerateDeviceExtensionProperties();

  std::vector<const char *> enabledExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME, "VK_KHR_portability_subset"};

  for (auto &req : enabledExtensions) {
    bool extFound = false;
    for (auto &i : physicalDevice.enumerateDeviceExtensionProperties()) {

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

  device =
      std::make_unique<vk::Device>(physicalDevice.createDevice(createInfo));
  log("succesfully created logical device", Severity::INFO);

  graphicsQueue = device->getQueue(queueFamilyIndices["Graphics"].value(), 0);
  presentQueue = device->getQueue(queueFamilyIndices["Present"].value(), 0);
}

vk::SurfaceFormatKHR Application::chooseFormat() {
  vk::Bool32 supported;

  vk::Result res = physicalDevice.getSurfaceSupportKHR(
      queueFamilyIndices["Graphics"].value(), surface, &supported);

  if (!supported) {
    assert(false);
  }

  auto availableFormats = physicalDevice.getSurfaceFormatsKHR(surface);
  for (auto &availableFormat : availableFormats) {
    if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
        availableFormat.colorSpace ==
            vk::ColorSpaceKHR::eDisplayNativeAMD) // im not sure why but this
                                                  // yields the best results
    {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

vk::Extent2D Application::chooseExtent() {

  auto capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
  swapChainImageCount = capabilities.minImageCount + 1;

  if (capabilities.currentExtent.width !=
      std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    SDL_GL_GetDrawableSize(window, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

void Application::createGraphicsPipeline() {

  m_PipelineLayout = std::make_unique<PipelineLayout>(device.get());

  vk::PushConstantRange pushConstantRange{

  };
  pushConstantRange.offset = 0;

  pushConstantRange.size = sizeof(MeshPushConstants);
  pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;

  m_PipelineLayout->addPushConstantRange(pushConstantRange);

  m_graphicsPipeline = std::make_unique<GraphicsPipeline>(
      "shaders/frag.spv", "shaders/vert.spv", device.get(), m_Renderpass->get(),
      m_PipelineLayout->get(),
      Vector2{(float)swapChainExtent.width, (float)swapChainExtent.height});
}

void Application::createCommandPool() {
  vk::CommandPoolCreateInfo poolInfo{};
  poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  poolInfo.queueFamilyIndex = queueFamilyIndices["Graphics"].value();

  assert(device->createCommandPool(&poolInfo, nullptr, &commandPool) ==
         vk::Result::eSuccess);
}

void Application::createVertexBuffer() {

  m_vertexBuffer = std::make_unique<vividX::VertexBuffer>(
      device.get(), sizeof(vertices[0]) * vertices.size(), physicalDevice);

  m_vertexBuffer->update(vertices);
}

void Application::createCommandBuffer() {
  vk::CommandBufferAllocateInfo info{};
  info.commandPool = commandPool;
  info.level = vk::CommandBufferLevel::ePrimary;
  info.commandBufferCount = 1;

  assert(device->allocateCommandBuffers(&info, &commandBuffer) ==
         vk::Result::eSuccess);
}

void Application::createSyncObjects() {
  vk::SemaphoreCreateInfo sephInfo{};
  vk::FenceCreateInfo fenceInfo{};
  fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

  vk::resultCheck(
      device->createSemaphore(&sephInfo, nullptr, &imageAvailableSeph),
      "failed to create imageSemaphore");
  vk::resultCheck(
      device->createSemaphore(&sephInfo, nullptr, &renderFinishedSeph),
      "Failed to create renderfinished semaphore");

  vk::resultCheck(device->createFence(&fenceInfo, nullptr, &inFlightfence),
                  "failed to create inFlight fence");
}
static glm::vec4 colour = {1, 1, 1, 1.0f};

void Application::recordCommandBuffer(uint32_t imageIndex) {
  vk::CommandBufferBeginInfo beginInfo{};
  beginInfo.pInheritanceInfo = nullptr; // Optional

  assert(commandBuffer.begin(&beginInfo) == vk::Result::eSuccess);

  vk::RenderPassBeginInfo renderPassInfo{};
  renderPassInfo.renderPass = m_Renderpass->get();
  renderPassInfo.framebuffer = m_SwapChain->getFrameBuffer(imageIndex);

  renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
  renderPassInfo.renderArea.extent = swapChainExtent;

  vk::ClearValue clearColour{};
  clearColour.color = {0.0f, 0.0f, 0.0f, 0.0f};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColour;

  commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                             m_graphicsPipeline->get());

  MeshPushConstants pushconstants;
  pushconstants.data = colour;
  pushconstants.render_matrix = cam.getViewProjMatrix();
  commandBuffer.pushConstants(m_PipelineLayout->get(),
                              vk::ShaderStageFlagBits::eVertex, 0,
                              sizeof(pushconstants), &pushconstants);

  vk::DeviceSize offsets = {0};
  auto a = m_vertexBuffer->getBuffer();

  commandBuffer.bindVertexBuffers(0, 1, &a, &offsets);

  commandBuffer.draw(3, 1, 0, 0);
  ImGui::Render();
  auto draw_data = ImGui::GetDrawData();

  const bool is_minimized =
      (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
  if (!is_minimized) {
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
  }

  commandBuffer.endRenderPass();
  commandBuffer.end();
}

void Application::drawFrame() {

  // auto res =
  //     device->waitForFences(1, &inFlightfence, vk::Bool32(true), UINT64_MAX);
  // vk::resultCheck(res, "waitForfences failed");

  // res = device->resetFences(1, &inFlightfence);
  // vk::resultCheck(res, "resetFences failed");
  // ImGui_ImplVulkan_NewFrame();
  // ImGui_ImplSDL2_NewFrame();
  // ImGui::NewFrame();

  // ImGui::ShowDemoWindow();

  // vk::AcquireNextImageInfoKHR info;
  // info.swapchain = swapChain;
  // info.semaphore = imageAvailableSeph;
  // info.timeout = UINT64_MAX;

  // auto imageIndex = device->acquireNextImageKHR(m_SwapChain->get(),
  // UINT64_MAX,
  //                                               imageAvailableSeph);

  // vk::resultCheck(imageIndex.result, "");

  // commandBuffer.reset();
  // recordCommandBuffer(imageIndex.value);

  // vk::SubmitInfo submitInfo{};

  // vk::Semaphore waitSemaphores[] = {imageAvailableSeph};

  // vk::PipelineStageFlags waitStages[] = {
  //     vk::PipelineStageFlagBits::eColorAttachmentOutput};
  // submitInfo.waitSemaphoreCount = 1;
  // submitInfo.pWaitSemaphores = waitSemaphores;
  // submitInfo.pWaitDstStageMask = waitStages;

  // submitInfo.commandBufferCount = 1;
  // submitInfo.pCommandBuffers = &commandBuffer;

  // vk::Semaphore signalSephamores[] = {renderFinishedSeph};
  // submitInfo.signalSemaphoreCount = 1;
  // submitInfo.pSignalSemaphores = signalSephamores;

  // res = graphicsQueue.submit(1, &submitInfo, inFlightfence);
  // assert(res == vk::Result::eSuccess);

  // vk::SubpassDependency dep{};
  // dep.srcSubpass = VK_SUBPASS_EXTERNAL;
  // dep.dstSubpass = 0;

  // dep.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

  // dep.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  // dep.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

  // vk::PresentInfoKHR presentInfo{};

  // vk::SwapchainKHR swapChains[] = {m_SwapChain->get()};

  // presentInfo.waitSemaphoreCount = 1;
  // presentInfo.pWaitSemaphores = signalSephamores;
  // presentInfo.swapchainCount = 1;
  // presentInfo.pSwapchains = swapChains;
  // presentInfo.pImageIndices = &imageIndex.value;
  // presentInfo.pResults = nullptr;
  // res = presentQueue.presentKHR(&presentInfo);
  // assert(res == vk::Result::eSuccess);

  renderer->beginFrame();

  renderer->drawFrame();

  renderer->endFrame();
}
void Application::cleanup() {

  ImGui_ImplVulkan_Shutdown();



  SDL_DestroyWindow(window);
  SDL_Quit();
}
