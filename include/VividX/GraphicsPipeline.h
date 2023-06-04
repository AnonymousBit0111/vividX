#pragma once
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <string>
namespace vividX {

class GraphicsPipeline {

private:


vk::Device *p_device = nullptr;
public:
  GraphicsPipeline(const std::string& fsPath,const std::string &vsPath, vk::Device *device,vk::VertexInputBindingDescription bindingDesc,vk::VertexInputAttributeDescription attributeDesc);
  ~GraphicsPipeline();
};
} // namespace vividX