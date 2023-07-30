#include "VividX/UniformBuffer.h"
#include "VividX/Globals.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <cstring>
using namespace vividX;

vk::DescriptorSetLayoutBinding
    UniformBuffer::uniformBinding(0, vk::DescriptorType::eUniformBuffer, 1,
                                  vk::ShaderStageFlagBits::eAllGraphics,
                                  nullptr);

UniformBuffer::UniformBuffer(uint32_t size) {

  TracyFunction;
  ZoneScoped;

  vk::BufferCreateInfo bufferInfo{};
  bufferInfo.size = size;
  bufferInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
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

void UniformBuffer::update(const UniformBufferObject ubo, uint32_t size) {
  TracyFunction;
  ZoneScoped;
  assert(sizeof(ubo.models[0])*size <= m_size);
//   memcpy(m_data, vertices.data(), vertices.size() * sizeof(vertices[0]));

  memcpy(m_data, ubo.models, sizeof(ubo.models[0])*size);
}