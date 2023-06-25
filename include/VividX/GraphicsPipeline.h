#pragma once
#include "VividX/PipelineLayout.h"
#include "vividx.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <string>
#include <vector>
namespace vividX {

class GraphicsPipeline {

private:

  vk::VertexInputBindingDescription m_bindingDesc;
  std::vector<vk::VertexInputAttributeDescription> m_attributeDescriptions;
  vk::Pipeline m_graphicsPipeline;

public:
  GraphicsPipeline(const std::string &fsPath, const std::string &vsPath,
                   vk::RenderPass rp, vk::PipelineLayout pipelineLayout,
                   vividX::Vector2 size);

  vk::Pipeline get() const { return m_graphicsPipeline; }
  ~GraphicsPipeline();
};
} // namespace vividX