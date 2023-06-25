#pragma once

#include "VividX/Globals.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <memory>
namespace vividX {

class RenderPass {

private:
  vk::RenderPass m_renderPass;


public:
  RenderPass(vk::SurfaceFormatKHR format);
  vk::RenderPass get() { return m_renderPass; }
  ~RenderPass() { g_vkContext->device.destroyRenderPass(m_renderPass); }
};

} // namespace vividX