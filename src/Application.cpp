#include "VividX/Application.h"
#include "SDL2/SDL_stdinc.h"
#include "SDL2/SDL_video.h"
#include "VkBootstrap.h"
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
const int WIDTH = 800;
const int HEIGHT = 600;

using namespace vividX;
// TODO change sephamore to semaphore
void Application::run() {

  vertices = vertices = {{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
                         {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
                         {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}};
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
  swapChainSurfaceFormat = chooseFormat();
  presentMode = choosePresentMode();
  swapChainExtent = chooseExtent();
  createSwapChain();
  createVertexBuffer();
  createCommandPool();
  createCommandBuffer();

  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();

  createSyncObjects();
}

void Application::initImGui() {

  ImGui::CreateContext();
  ImGuiDescriptorPool = device->createDescriptorPool(poolInfo);

  ImGui_ImplVulkan_InitInfo initInfo{};
  initInfo.Allocator = nullptr;
  initInfo.Instance = instance;
  initInfo.ImageCount = swapChainFrameBuffers.size();
  initInfo.MinImageCount = 2;
  initInfo.Queue = graphicsQueue;

  initInfo.QueueFamily = queueFamilyIndices["Graphics"].value();
  initInfo.Device = *device;
  initInfo.PhysicalDevice = physicalDevice;
  initInfo.CheckVkResultFn = nullptr;
  initInfo.DescriptorPool = ImGuiDescriptorPool;
  initInfo.PipelineCache = {};

  ImGui_ImplSDL2_InitForVulkan(window);
  ImGui_ImplVulkan_Init(&initInfo, renderPass);

  vk::CommandBufferAllocateInfo allocateInfo = {
      commandPool,                      // Command pool
      vk::CommandBufferLevel::ePrimary, // Command buffer level
      1                                 // Number of command buffers to allocate
  };
  vk::CommandBuffer tempBuffer;

  vk::resultCheck(device->allocateCommandBuffers(&allocateInfo, &tempBuffer),
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
  device->waitIdle();
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
            vk::ColorSpaceKHR::eDisplayNativeAMD) // im not sure why but this yields the best results
            {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

vk::PresentModeKHR Application::choosePresentMode() {
  std::vector<vk::PresentModeKHR> modes =
      physicalDevice.getSurfacePresentModesKHR(surface);
  for (const auto &availablePresentMode : modes) {
    if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
      return availablePresentMode;
    }
  }

  return vk::PresentModeKHR::eFifo;
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

void Application::createSwapChain() {
  vk::SwapchainCreateInfoKHR createInfo{};
  createInfo.surface = surface;
  createInfo.minImageCount = swapChainImageCount;

  createInfo.imageFormat = swapChainSurfaceFormat.format;
  createInfo.imageColorSpace = swapChainSurfaceFormat.colorSpace;
  createInfo.imageExtent = swapChainExtent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
  createInfo.presentMode = presentMode;
  createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
  createInfo.preTransform =
      physicalDevice.getSurfaceCapabilitiesKHR(surface).currentTransform;

  createInfo.clipped = 1;
  createInfo.oldSwapchain = vk::SwapchainKHR{};

  auto res = device->createSwapchainKHR(&createInfo, nullptr, &swapChain);
  assert(res == vk::Result::eSuccess);
  swapChainImages = device->getSwapchainImagesKHR(swapChain);

  swapChainImageViews.resize(swapChainImages.size());

  int index = 0;
  for (auto &i : swapChainImages) {
    vk::ImageViewCreateInfo info{};
    info.image = i;
    info.viewType = vk::ImageViewType::e2D;
    info.format = swapChainSurfaceFormat.format;

    info.components.r = vk::ComponentSwizzle::eIdentity;
    info.components.g = vk::ComponentSwizzle::eIdentity;
    info.components.b = vk::ComponentSwizzle::eIdentity;
    info.components.a = vk::ComponentSwizzle::eIdentity;

    info.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;

    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;
    vk::Result res =
        device->createImageView(&info, nullptr, &swapChainImageViews[index]);
    if (res != vk::Result::eSuccess) {
      assert(false);
    }
    index++;
  }
}

void Application::createRenderPass() {
  vk::AttachmentDescription colourAttachment{};
  colourAttachment.format = swapChainSurfaceFormat.format;
  colourAttachment.samples = vk::SampleCountFlagBits::e1;

  colourAttachment.loadOp = vk::AttachmentLoadOp::eClear;
  colourAttachment.storeOp = vk::AttachmentStoreOp::eStore;

  colourAttachment.initialLayout = vk::ImageLayout::eUndefined;
  colourAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;
  vk::AttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

  vk::SubpassDescription subpass{};
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colourAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  auto res = device->createRenderPass(&renderPassInfo, nullptr, &renderPass);

  vk::resultCheck(res, "error, renderPass creation failed");
}

void Application::createGraphicsPipeline() {
  auto vertShaderCode = readFile("shaders/vert.spv");
  auto fragShaderCode = readFile("shaders/frag.spv");
  vk::ShaderModule fragModule = createShaderModule(fragShaderCode, *device);
  vk::ShaderModule vertModule = createShaderModule(vertShaderCode, *device);

  vk::PipelineShaderStageCreateInfo vertStageCreateInfo{};
  vertStageCreateInfo.stage = vk::ShaderStageFlagBits::eVertex;
  vertStageCreateInfo.module = vertModule;
  vertStageCreateInfo.pName = "main";

  vk::PipelineShaderStageCreateInfo fragStageCreateInfo{};
  fragStageCreateInfo.stage = vk::ShaderStageFlagBits::eFragment;
  fragStageCreateInfo.module = fragModule;
  fragStageCreateInfo.pName = "main";

  vk::PipelineShaderStageCreateInfo shaderStages[] = {vertStageCreateInfo,
                                                      fragStageCreateInfo};

  vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
  auto bindingDesc = getBindingDescription();

  auto attribDesc = getAttribDesc();
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;

  vertexInputInfo.vertexAttributeDescriptionCount = attribDesc.size();
  vertexInputInfo.pVertexAttributeDescriptions = attribDesc.data();

  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  vk::Viewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)swapChainExtent.width;
  viewport.height = (float)swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vk::Rect2D scissor{};
  scissor.offset = vk::Offset2D{0, 0};
  scissor.extent = swapChainExtent;

  vk::PipelineViewportStateCreateInfo viewportState{};
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  vk::PipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = vk::PolygonMode::eFill;
  rasterizer.lineWidth = 1.0f;

  rasterizer.cullMode = vk::CullModeFlagBits::eBack;
  rasterizer.frontFace = vk::FrontFace::eClockwise;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f; // Optional
  rasterizer.depthBiasClamp = 0.0f;          // Optional
  rasterizer.depthBiasSlopeFactor = 0.0f;    // Optional

  vk::PipelineMultisampleStateCreateInfo multisampling{};

  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
  multisampling.minSampleShading = 1.0f;          // Optional
  multisampling.pSampleMask = nullptr;            // Optional
  multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
  multisampling.alphaToOneEnable = VK_FALSE;      // Optional

  vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
  colorBlendAttachment.blendEnable = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne;
  colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero;
  colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd;             // Optional
  colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;  // Optional
  colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero; // Optional
  colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd;             // Optional

  vk::PipelineColorBlendStateCreateInfo colourBlendInfo{};
  colourBlendInfo.pAttachments = &colorBlendAttachment;
  colourBlendInfo.attachmentCount = 1;
  colourBlendInfo.logicOp = vk::LogicOp::eCopy;
  colourBlendInfo.logicOpEnable = vk::Bool32(false);

  colourBlendInfo.blendConstants[0] = 1;
  colourBlendInfo.blendConstants[1] = 1;
  colourBlendInfo.blendConstants[2] = 1;
  colourBlendInfo.blendConstants[3] = 1;

  vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo{};

  if (device->createPipelineLayout(&pipelineLayoutCreateInfo, nullptr,
                                   &pipelineLayout) != vk::Result::eSuccess) {
    assert(false);
  }

  vk::GraphicsPipelineCreateInfo pipelineCreateInfo{};

  pipelineCreateInfo.stageCount = 2;
  pipelineCreateInfo.pStages = shaderStages;

  pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
  pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
  pipelineCreateInfo.pViewportState = &viewportState;
  pipelineCreateInfo.pRasterizationState = &rasterizer;
  pipelineCreateInfo.pMultisampleState = &multisampling;
  pipelineCreateInfo.pDepthStencilState = nullptr; // Optional

  pipelineCreateInfo.pColorBlendState = &colourBlendInfo;
  // pipelineCreateInfo.pDynamicState = &dynamicState;
  pipelineCreateInfo.layout = pipelineLayout;

  pipelineCreateInfo.renderPass = renderPass;
  pipelineCreateInfo.subpass = 0;

  if (device->createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineCreateInfo,
                                      nullptr, &graphicsPipeline) !=
      vk::Result::eSuccess) {
    assert(false);
  }

  device->destroyShaderModule(fragModule);
  device->destroyShaderModule(vertModule);
}

void Application::createFramebuffers() {
  swapChainFrameBuffers.resize(swapChainImageViews.size());

  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    vk::ImageView attachments[] = {swapChainImageViews[i]};
    vk::FramebufferCreateInfo frameBufferInfo{};
    frameBufferInfo.renderPass = renderPass;
    frameBufferInfo.attachmentCount = 1;
    frameBufferInfo.pAttachments = attachments;
    frameBufferInfo.width = swapChainExtent.width;
    frameBufferInfo.height = swapChainExtent.height;
    frameBufferInfo.layers = 1;
    assert(device->createFramebuffer(&frameBufferInfo, nullptr,
                                     &swapChainFrameBuffers[i]) ==
           vk::Result::eSuccess);
  }
}

void Application::createCommandPool() {
  vk::CommandPoolCreateInfo poolInfo{};
  poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
  poolInfo.queueFamilyIndex = queueFamilyIndices["Graphics"].value();

  assert(device->createCommandPool(&poolInfo, nullptr, &commandPool) ==
         vk::Result::eSuccess);
}

void Application::createVertexBuffer() {
  vk::BufferCreateInfo bufferInfo{};
  bufferInfo.size = sizeof(PosColourVertex) * vertices.size();
  bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
  bufferInfo.sharingMode = vk::SharingMode::eExclusive;

  vk::resultCheck(device->createBuffer(&bufferInfo, nullptr, &vertexBuffer),
                  "error creating vkbuffer");
  vk::MemoryRequirements requirements =
      device->getBufferMemoryRequirements(vertexBuffer);
  vk::MemoryAllocateInfo memInfo{};
  memInfo.allocationSize = requirements.size;
  memInfo.memoryTypeIndex =
      findMemoryType(requirements.memoryTypeBits,
                     vk::MemoryPropertyFlagBits::eHostVisible |
                         vk::MemoryPropertyFlagBits::eHostCoherent);

  vk::resultCheck(
      device->allocateMemory(&memInfo, nullptr, &vertexBufferMemory), "");
  device->bindBufferMemory(vertexBuffer, vertexBufferMemory, 0);

  void *data = device->mapMemory(vertexBufferMemory, 0, bufferInfo.size);
  memcpy(data, vertices.data(), bufferInfo.size);
  device->unmapMemory(vertexBufferMemory);
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

void Application::recordCommandBuffer(uint32_t imageIndex) {
  vk::CommandBufferBeginInfo beginInfo{};
  beginInfo.pInheritanceInfo = nullptr; // Optional

  assert(commandBuffer.begin(&beginInfo) == vk::Result::eSuccess);

  vk::RenderPassBeginInfo renderPassInfo{};
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = swapChainFrameBuffers[imageIndex];

  renderPassInfo.renderArea.offset = vk::Offset2D{0, 0};
  renderPassInfo.renderArea.extent = swapChainExtent;

  vk::ClearValue clearColour{};
  clearColour.color = {0.0f, 0.0f, 0.0f, 0.0f};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColour;

  commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);
  commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                             graphicsPipeline);

  vk::DeviceSize offsets = {0};

  commandBuffer.bindVertexBuffers(0, 1, &vertexBuffer, &offsets);

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

  auto res =
      device->waitForFences(1, &inFlightfence, vk::Bool32(true), UINT64_MAX);
  vk::resultCheck(res, "waitForfences failed");

  res = device->resetFences(1, &inFlightfence);
  vk::resultCheck(res, "resetFences failed");
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  ImGui::ShowDemoWindow();
  vk::AcquireNextImageInfoKHR info;
  info.swapchain = swapChain;
  info.semaphore = imageAvailableSeph;
  info.timeout = UINT64_MAX;

  auto imageIndex =
      device->acquireNextImageKHR(swapChain, UINT64_MAX, imageAvailableSeph);
  assert(imageIndex.result == vk::Result::eSuccess);

  commandBuffer.reset();
  recordCommandBuffer(imageIndex.value);

  vk::SubmitInfo submitInfo{};

  vk::Semaphore waitSemaphores[] = {imageAvailableSeph};

  vk::PipelineStageFlags waitStages[] = {
      vk::PipelineStageFlagBits::eColorAttachmentOutput};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vk::Semaphore signalSephamores[] = {renderFinishedSeph};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSephamores;

  res = graphicsQueue.submit(1, &submitInfo, inFlightfence);
  assert(res == vk::Result::eSuccess);

  vk::SubpassDependency dep{};
  dep.srcSubpass = VK_SUBPASS_EXTERNAL;
  dep.dstSubpass = 0;

  dep.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

  dep.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
  dep.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dep;

  vk::PresentInfoKHR presentInfo{};

  vk::SwapchainKHR swapChains[] = {swapChain};

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSephamores;
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex.value;
  presentInfo.pResults = nullptr;
  res = presentQueue.presentKHR(&presentInfo);
  assert(res == vk::Result::eSuccess);
}
void Application::cleanup() {

  ImGui_ImplVulkan_Shutdown();

  for (auto &i : swapChainFrameBuffers) {
    device->destroy(i);
  }
  for (auto imageView : swapChainImageViews) {
    device->destroyImageView(imageView);
  }
  device->freeMemory(vertexBufferMemory);

  device->destroyBuffer(vertexBuffer);
  device->destroyFence(inFlightfence);
  device->destroySemaphore(renderFinishedSeph);
  device->destroySemaphore(imageAvailableSeph);

  device->destroyCommandPool(commandPool);
  device->destroyPipelineLayout(pipelineLayout);
  device->destroyRenderPass(renderPass);
  device->destroyPipeline(graphicsPipeline);
  device->destroySwapchainKHR(swapChain);
  device->destroyDescriptorPool(ImGuiDescriptorPool);
  device->destroy();
  instance.destroySurfaceKHR(surface);

  SDL_DestroyWindow(window);
  SDL_Quit();
}

vk::VertexInputBindingDescription Application::getBindingDescription() {
  vk::VertexInputBindingDescription bindingDescription{};
  bindingDescription.binding = 0;
  bindingDescription.stride = sizeof(PosColourVertex);
  bindingDescription.inputRate = vk::VertexInputRate::eVertex;

  return bindingDescription;
}

std::array<vk::VertexInputAttributeDescription, 2>
Application::getAttribDesc() {
  vk::VertexInputAttributeDescription Pos{};
  Pos.binding = 0;
  Pos.location = 0;
  Pos.format = vk::Format::eR32G32Sfloat;
  Pos.offset = offsetof(PosColourVertex, pos);

  vk::VertexInputAttributeDescription Colour{};
  Colour.binding = 0;
  Colour.location = 1;
  Colour.format = vk::Format::eR32G32B32Sfloat;
  Colour.offset = offsetof(PosColourVertex, colour);

  return {Pos, Colour};
}

uint32_t Application::findMemoryType(uint32_t typeFilter,
                                     vk::MemoryPropertyFlags properties) {
  vk::PhysicalDeviceMemoryProperties memProps{};

  physicalDevice.getMemoryProperties(&memProps);

  for (unsigned int i = 0; i < memProps.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  log("Unable to find suitable memory type", Severity::ERROR);
  return -1;
}
