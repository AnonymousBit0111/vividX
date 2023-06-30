#pragma once
#include <_types/_uint32_t.h>
#include <vulkan/vulkan.hpp>
namespace vividX {

class IndexBuffer {

private:
  vk::Buffer m_rawBuffer;
  vk::DeviceMemory m_memory;
  uint32_t m_size;
  void *m_data;

public:
  IndexBuffer(uint32_t size);
  void update(std::vector<uint32_t> indices);
  vk::Buffer get() const { return m_rawBuffer; }
  ~IndexBuffer();
};
} // namespace vividX