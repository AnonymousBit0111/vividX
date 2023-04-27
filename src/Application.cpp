#include "VividX/Application.h"
#include "SDL2/SDL_video.h"
#include "VkBootstrap.h"
#include "vividx.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <_types/_uint32_t.h>
#include <cassert>
#include <iostream>
#include <optional>
#include <vector>
const int WIDTH = 800;
const int HEIGHT = 600;

using namespace vividX;

void Application::run() {
  initSDL();
  initVulkan();
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

  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  createCommandPool();
  createCommandBuffer();
}

void Application::createInstance() {
  vk::ApplicationInfo appInfo{};
  appInfo.pApplicationName = "Vulkan SDL2 Demo";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  vk::InstanceCreateInfo createInfo{};
  createInfo.pApplicationInfo = &appInfo;

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
  for (auto &i : extensions) {
    log(i, Severity::INFO);
  }
  createInfo.ppEnabledExtensionNames = extensions.data();

  if (vk::createInstance(&createInfo, nullptr, &instance) !=
      vk::Result::eSuccess) {
    assert(false && "Failed to create Vulkan instance.");
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
    }
    drawFrame();
  }
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

  int i = 0;
  for (const auto &queuefamily : queueFamilies) {
    if (queuefamily.queueFlags & vk::QueueFlagBits::eGraphics) {
      queueFamilyIndices["Graphics"] = i;
    }
    i++;
  }

  vk::DeviceQueueCreateInfo info{};
  info.queueFamilyIndex = queueFamilyIndices["Graphics"].value();
  info.queueCount = 1;
  static float queuePriority = 1.0f;
  info.pQueuePriorities = &queuePriority;

  vk::PhysicalDeviceFeatures devFeatures{};

  std::vector<vk::ExtensionProperties> extensions =
      physicalDevice.enumerateDeviceExtensionProperties();

  std::vector<const char *> extNames;
  for (auto &i : extensions) {
    extNames.push_back(i.extensionName);
  }

  vk::DeviceCreateInfo createInfo{};
  extNames.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

  createInfo.setEnabledExtensionCount(extNames.size());
  createInfo.setPEnabledExtensionNames(extNames);
  createInfo.setEnabledLayerCount(0);

  device = physicalDevice.createDeviceUnique(createInfo);

  std::string message = "the following extensions are enabled:";
  for (auto &i : extNames) {
    message += i;
    message += "\n";
  }
  log(message, Severity::INFO);
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
            vk::ColorSpaceKHR::eVkColorspaceSrgbNonlinear) {
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

  swapChain = device->createSwapchainKHR(createInfo);

  swapChainImages = device->getSwapchainImagesKHR(swapChain);

  swapChainImageViews.resize(swapChainImages.size());

  int index = 0;
  for (auto &i : swapChainImages) {
    vk::ImageViewCreateInfo info{};
    info.image = i;
    info.viewType = vk::ImageViewType::e2D;
    info.format = physicalDevice.getSurfaceFormatsKHR(surface)[0].format;
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
  colourAttachment.finalLayout = vk::ImageLayout::eAttachmentOptimal;

  vk::AttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

  vk::SubpassDescription subpass{};
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  vk::RenderPassCreateInfo renderPassInfo{};
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colourAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  if (device->createRenderPass(&renderPassInfo, nullptr, &renderPass) !=
      vk::Result::eSuccess) {
    assert(false);
  }
}

void Application::createGraphicsPipeline() {
  auto vertShaderCode = readFile("shaders/vert.spv");
  auto fragShaderCode = readFile("shaders/frag.spv");
  vk::ShaderModule fragModule = createShaderModule(fragShaderCode, device);
  vk::ShaderModule vertModule = createShaderModule(vertShaderCode, device);

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

  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

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

  rasterizer.cullMode = vk::CullModeFlagBits::eFront;
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
  // pipelineCreateInfo.pColorBlendState = &colorBlendAttachment;
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

void Application::createCommandBuffer() {
  vk::CommandBufferAllocateInfo info{};
  info.commandPool = commandPool;
  info.level = vk::CommandBufferLevel::ePrimary;
  info.commandBufferCount = 1;

  assert(device->allocateCommandBuffers(&info, &commandBuffer) ==
         vk::Result::eSuccess);
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
  clearColour.color = {1.0f, 1.0f, 1.0f, 1.0f};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearColour;

  commandBuffer.beginRenderPass(&renderPassInfo, vk::SubpassContents::eInline);

  commandBuffer.draw(3, 1, 0, 0);

  commandBuffer.endRenderPass();
  commandBuffer.end();
}

void Application::drawFrame() {
  recordCommandBuffer(0);
}
void Application::cleanup() {

  for (auto &i : swapChainFrameBuffers) {
    device->destroy(i);
  }
  for (auto imageView : swapChainImageViews) {
    device->destroyImageView(imageView);
  }
  device->destroyCommandPool(commandPool);
  device->destroyPipelineLayout(pipelineLayout);
  device->destroyRenderPass(renderPass);
  instance.destroySurfaceKHR(surface);
  instance.destroy();

  SDL_DestroyWindow(window);
  SDL_Quit();
}
