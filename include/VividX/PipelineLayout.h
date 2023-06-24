#pragma once

#include "vulkan/vulkan.hpp"
namespace vividX {
class PipelineLayout {

private:
  vk::Device *p_device;
  vk::PipelineLayout m_pipelineLayout;
  std::vector<vk::DescriptorSetLayout> m_descriptorSetLayouts;
  std::vector<vk::PushConstantRange> m_pushConstantRanges;
  vk::PipelineLayoutCreateInfo m_createInfo{};

  void recreate();

public:
  PipelineLayout(vk::Device *device);
  ~PipelineLayout();

  void
  addDescriptorSetLayout(const vk::DescriptorSetLayout &descriptorSetLayout);
  void addPushConstantRange(const vk::PushConstantRange &pushConstantRange);

  vk::PipelineLayout get() const { return m_pipelineLayout; }
};
} // namespace vividX