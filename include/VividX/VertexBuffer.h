#pragma once

#include "VividX/Globals.h"
#include "vividx.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include <_types/_uint32_t.h>
#include <vector>
namespace vividX {

class VertexBuffer {

private:
  vk::Buffer m_rawBuffer;
  vk::DeviceMemory m_memory;
  uint32_t m_size;
  void *m_data;

public:
  VertexBuffer(uint32_t size);
  ~VertexBuffer() {
    g_vkContext->device.destroyBuffer(m_rawBuffer);
    g_vkContext->device.freeMemory(m_memory);
  }
  vk::Buffer getBuffer() { return m_rawBuffer; }
  vk::Buffer &getBufferRef() { return m_rawBuffer; }

  void update(const std::vector<PosColourVertex> &vertices);
  vk::DeviceMemory getMemory() { return m_memory; }
};
} // namespace vividX