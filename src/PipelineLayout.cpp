

#include "VividX/PipelineLayout.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"

using namespace vividX;

void PipelineLayout::recreate() {

  m_createInfo.pPushConstantRanges = m_pushConstantRanges.data();
  m_createInfo.pushConstantRangeCount = m_pushConstantRanges.size();
  m_createInfo.pSetLayouts = m_descriptorSetLayouts.data();
  m_createInfo.setLayoutCount = m_descriptorSetLayouts.size();
  if (m_pipelineLayout) {
    p_device->destroyPipelineLayout(m_pipelineLayout);
  }

  m_pipelineLayout = p_device->createPipelineLayout(m_createInfo);
}

PipelineLayout::PipelineLayout(vk::Device *device) : p_device(device) {
  assert(p_device->createPipelineLayout(&m_createInfo, nullptr,
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
  p_device->destroyPipelineLayout(m_pipelineLayout);
}