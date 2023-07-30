#include "VividX/IndexBuffer.h"
#include <_types/_uint32_t.h>

#include "VividX/Globals.h"
#include "vividx.h"
#include "vulkan/vulkan.hpp"

using namespace vividX;

IndexBuffer::IndexBuffer(uint32_t size) : m_size(size) {
  TracyFunction;
  ZoneScoped;

  vk::BufferCreateInfo bufferInfo{};
  bufferInfo.size = size * sizeof(uint32_t);
  bufferInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer;
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

void IndexBuffer::update(const std::vector<uint32_t> indices) {
  TracyFunction;
  ZoneScoped;
  assert(indices.size() <= m_size);
  memcpy(m_data, indices.data(), indices.size() * sizeof(indices[0]));
}

IndexBuffer::~IndexBuffer() {
  TracyFunction;
  ZoneScoped;
  g_vkContext->device.destroyBuffer(m_rawBuffer);
  g_vkContext->device.freeMemory(m_memory);
}