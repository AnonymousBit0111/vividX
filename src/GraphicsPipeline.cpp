#include "VividX/GraphicsPipeline.h"
#include "VividX/Globals.h"
#include "VividX/PipelineLayout.h"
#include "vividx.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <cassert>
using namespace vividX;
GraphicsPipeline::GraphicsPipeline(const std::string &fsPath,
                                   const std::string &vsPath, vk::RenderPass rp,
                                   vk::PipelineLayout layout,
                                   Vector2 viewportSize) {
  TracyFunction;
  ZoneScoped;

  auto vertShaderCode = readFile(vsPath);
  auto fragShaderCode = readFile(fsPath);
  vk::ShaderModule fragModule =
      createShaderModule(fragShaderCode, g_vkContext->device);
  vk::ShaderModule vertModule =
      createShaderModule(vertShaderCode, g_vkContext->device);

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

  m_bindingDesc = vividX::getBindingDescription();

  m_attributeDescriptions = vividX::getAttribDesc();

  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &m_bindingDesc;

  vertexInputInfo.vertexAttributeDescriptionCount =
      m_attributeDescriptions.size();
  vertexInputInfo.pVertexAttributeDescriptions = m_attributeDescriptions.data();

  vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  vk::Viewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = viewportSize.x;
  viewport.height = viewportSize.y;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  vk::Rect2D scissor{};
  scissor.offset = vk::Offset2D{0, 0};
  scissor.extent.width = viewportSize.x;
  scissor.extent.height = viewportSize.y;

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
  pipelineCreateInfo.layout = layout;

  pipelineCreateInfo.renderPass = rp;

  vk::Result res = g_vkContext->device.createGraphicsPipelines(
      VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &m_graphicsPipeline);

  vk::resultCheck(res, "failed");

  g_vkContext->device.destroyShaderModule(fragModule);
  g_vkContext->device.destroyShaderModule(vertModule);
}

GraphicsPipeline::~GraphicsPipeline() {
  TracyFunction;
  ZoneScoped;
  g_vkContext->device.destroyPipeline(m_graphicsPipeline);
}