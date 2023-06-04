#pragma once

#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <memory>
namespace vividX {

class RenderPass {

private:
  vk::RenderPass m_renderPass;
  vk::Device *p_device;

public:
  RenderPass(vk::Device *dev, vk::SurfaceFormatKHR format);
  vk::RenderPass get() { return m_renderPass; }
  ~RenderPass() { p_device->destroyRenderPass(m_renderPass); }
};

} // namespace vividX