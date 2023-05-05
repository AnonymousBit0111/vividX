#pragma once

#include "vividx.h"
#include <_types/_uint32_t.h>
#include <memory>
#include <vulkan/vulkan.hpp>
namespace vividX {


class SwapChain {

private:
  std::unique_ptr<vk::SwapchainKHR> m_rawSwapChain;

  vk::Device * p_logicalDevice; // all objects must have a shorter lifetime than the logical device

public:
  SwapChain(uint32_t imageCount, Vector2i size);

  ~SwapChain();
};
} // namespace vividX