#pragma once

#include "vividx.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include <vector>
namespace vividX {

class VertexBuffer {

private:
  vk::Buffer m_rawBuffer;
  vk::DeviceMemory m_memory;
  vk::Device *p_device;
  uint32_t m_size;
  void *m_data;

public:
  VertexBuffer(vk::Device *dev, uint32_t size,
               vk::PhysicalDevice &PhysicalDevice);
  ~VertexBuffer() {
    p_device->destroyBuffer(m_rawBuffer);
    p_device->freeMemory(m_memory);
  }
  vk::Buffer getBuffer() { return m_rawBuffer; }

  void update(const std::vector<PosColourVertex> &vertices);
  vk::DeviceMemory getMemory() { return m_memory; }
};
} // namespace vividX