#pragma once
#include "vividx.h"
#include <_types/_uint32_t.h>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "VividX/Globals.h"
namespace vividX {

struct UniformBufferObject {
  glm::mat4 *models;


};
class UniformBuffer {

private:
  vk::Buffer m_rawBuffer;
  vk::DeviceMemory m_memory;
  uint32_t m_size;
  void *m_data;
  static vk::DescriptorSetLayoutBinding uniformBinding;

public:
  UniformBuffer(uint32_t size);

  static vk::DescriptorSetLayoutBinding getDefaultBinding() {
    return uniformBinding;
  }

  vk::Buffer getBuffer() { return m_rawBuffer; }
  vk::Buffer &getBufferRef() { return m_rawBuffer; }

  void update(const UniformBufferObject ubo,uint32_t size);
  vk::DeviceMemory getMemory() { return m_memory; }
  ~UniformBuffer() {
    g_vkContext->device.destroyBuffer(m_rawBuffer);
    g_vkContext->device.freeMemory(m_memory);
  }
};
} // namespace vividX