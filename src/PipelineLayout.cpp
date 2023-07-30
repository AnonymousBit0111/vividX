

#include "VividX/PipelineLayout.h"
#include "VividX/Globals.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"

#include "vividx.h"

using namespace vividX;

void PipelineLayout::recreate() {
  TracyFunction;
  ZoneScoped;

  m_createInfo.pPushConstantRanges = m_pushConstantRanges.data();
  m_createInfo.pushConstantRangeCount = m_pushConstantRanges.size();
  m_createInfo.pSetLayouts = m_descriptorSetLayouts.data();
  m_createInfo.setLayoutCount = m_descriptorSetLayouts.size();
  if (m_pipelineLayout) {
    g_vkContext->device.destroyPipelineLayout(m_pipelineLayout);
  }
  assert(g_vkContext->device.createPipelineLayout(&m_createInfo, nullptr,
                                                  &m_pipelineLayout) ==
             vk::Result::eSuccess ||
         "Failed To create pipeline");
}

PipelineLayout::PipelineLayout() {
  assert(g_vkContext->device.createPipelineLayout(&m_createInfo, nullptr,
                                                  &m_pipelineLayout) ==
             vk::Result::eSuccess ||
         "Failed To create pipeline");
}

// Note : this may be ineficient so consider adding descriptorsetlayouts in the
// constructor

void PipelineLayout::addDescriptorSetLayout(
    const vk::DescriptorSetLayout &descriptorSetLayout) {
  TracyFunction;
  ZoneScoped;
  m_descriptorSetLayouts.push_back(descriptorSetLayout);

  recreate();
}

void PipelineLayout::addPushConstantRange(
    const vk::PushConstantRange &pushConstantRange) {
  TracyFunction;
  ZoneScoped;
  m_pushConstantRanges.push_back(pushConstantRange);
  recreate();
}

PipelineLayout::~PipelineLayout() {
  TracyFunction;
  ZoneScoped;

  for (auto &i : m_descriptorSetLayouts) {
    g_vkContext->device.destroyDescriptorSetLayout(i);
  }
  g_vkContext->device.destroyPipelineLayout(m_pipelineLayout);
}