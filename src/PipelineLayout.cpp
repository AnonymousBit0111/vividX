

#include "VividX/PipelineLayout.h"
#include "VividX/Globals.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"

using namespace vividX;

void PipelineLayout::recreate() {

  m_createInfo.pPushConstantRanges = m_pushConstantRanges.data();
  m_createInfo.pushConstantRangeCount = m_pushConstantRanges.size();
  m_createInfo.pSetLayouts = m_descriptorSetLayouts.data();
  m_createInfo.setLayoutCount = m_descriptorSetLayouts.size();
  if (m_pipelineLayout) {
    g_vkContext->device.destroyPipelineLayout(m_pipelineLayout);
  }

  m_pipelineLayout = g_vkContext->device.createPipelineLayout(m_createInfo);
}

PipelineLayout::PipelineLayout() {
  assert(g_vkContext->device.createPipelineLayout(&m_createInfo, nullptr,
                                                  &m_pipelineLayout) ==
             vk::Result::eSuccess ||
         "Failed To create pipeline");
}

void PipelineLayout::addDescriptorSetLayout(
    const vk::DescriptorSetLayout &descriptorSetLayout) {
  m_descriptorSetLayouts.push_back(descriptorSetLayout);
  recreate();
}

void PipelineLayout::addPushConstantRange(
    const vk::PushConstantRange &pushConstantRange) {
  m_pushConstantRanges.push_back(pushConstantRange);
  recreate();
}

PipelineLayout::~PipelineLayout() {
  g_vkContext->device.destroyPipelineLayout(m_pipelineLayout);
}