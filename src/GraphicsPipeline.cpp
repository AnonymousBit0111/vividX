#include "VividX/GraphicsPipeline.h"
#include "vividx.h"
#include "vulkan/vulkan_handles.hpp"
using namespace vividX;
GraphicsPipeline::GraphicsPipeline(const std::string &fsPath,
                                   const std::string &vsPath,
                                   vk::Device *device)
    : p_device(device) {

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
}
