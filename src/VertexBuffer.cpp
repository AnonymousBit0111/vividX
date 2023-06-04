#include "VividX/VertexBuffer.h"
#include "vividx.h"
#include <cassert>

using namespace vividX;

VertexBuffer::VertexBuffer(vk::Device *dev, uint32_t size,
                           vk::PhysicalDevice &physicalDevice)
    : p_device(dev), m_size(size) {

  vk::BufferCreateInfo bufferInfo{};
  bufferInfo.size = size;
  bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
  bufferInfo.sharingMode = vk::SharingMode::eExclusive;

  vk::resultCheck(dev->createBuffer(&bufferInfo, nullptr, &m_rawBuffer),
                  "error creating vkbuffer");

  vk::MemoryRequirements requirements =
      dev->getBufferMemoryRequirements(m_rawBuffer);
  vk::MemoryAllocateInfo memInfo{};
  memInfo.allocationSize = requirements.size;
  memInfo.memoryTypeIndex =
      findMemoryType(requirements.memoryTypeBits,
                     vk::MemoryPropertyFlagBits::eHostVisible |
                         vk::MemoryPropertyFlagBits::eHostCoherent,
                     physicalDevice);

  vk::resultCheck(dev->allocateMemory(&memInfo, nullptr, &m_memory), "");

  dev->bindBufferMemory(m_rawBuffer, m_memory, 0);

  m_data = dev->mapMemory(m_memory, 0, bufferInfo.size);
}

void VertexBuffer::update(const std::vector<PosColourVertex> &vertices) {
  assert(vertices.size() * sizeof(vertices[0]) <= m_size);
  memcpy(m_data, vertices.data(), vertices.size() * sizeof(vertices[0]));
}
