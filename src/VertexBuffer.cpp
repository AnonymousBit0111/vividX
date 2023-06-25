#include "VividX/VertexBuffer.h"
#include "VividX/Globals.h"
#include "vividx.h"
#include <cassert>

using namespace vividX;

VertexBuffer::VertexBuffer(uint32_t size) : m_size(size) {

  vk::BufferCreateInfo bufferInfo{};
  bufferInfo.size = size;
  bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
  bufferInfo.sharingMode = vk::SharingMode::eExclusive;

  vk::resultCheck(
      g_vkContext->device.createBuffer(&bufferInfo, nullptr, &m_rawBuffer),
      "error creating vkbuffer");

  vk::MemoryRequirements requirements =
      g_vkContext->device.getBufferMemoryRequirements(m_rawBuffer);
  vk::MemoryAllocateInfo memInfo{};
  memInfo.allocationSize = requirements.size;
  memInfo.memoryTypeIndex =
      findMemoryType(requirements.memoryTypeBits,
                     vk::MemoryPropertyFlagBits::eHostVisible |
                         vk::MemoryPropertyFlagBits::eHostCoherent,
                     g_vkContext->physicalDevice);

  vk::resultCheck(
      g_vkContext->device.allocateMemory(&memInfo, nullptr, &m_memory), "");

  g_vkContext->device.bindBufferMemory(m_rawBuffer, m_memory, 0);

  m_data = g_vkContext->device.mapMemory(m_memory, 0, bufferInfo.size);
}

void VertexBuffer::update(const std::vector<PosColourVertex> &vertices) {
  assert(vertices.size() * sizeof(vertices[0]) <= m_size);
  memcpy(m_data, vertices.data(), vertices.size() * sizeof(vertices[0]));
}
