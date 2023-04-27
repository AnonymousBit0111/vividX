#pragma once

#include "SDL2/SDL_video.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_to_string.hpp"
#include <string>
#include <vector>

namespace vividX {

struct Vector3 {
  float x, y, z;
};
struct PosColourVertex {
  Vector3 pos;
  Vector3 color;
};

enum class Severity {

  INFO,
  WARNING,
  ERROR,
  FATAL
};
void log(const std::string &message, Severity severity);

vk::Instance createInstance();

vk::SurfaceKHR createWindowSurface(SDL_Window *window);

int initialiseVulkan(SDL_Window *window);

std::vector<char> readFile(const std::string &filename);
vk::ShaderModule createShaderModule(const std::vector<char> &code,vk::UniqueDevice &device);

VkBool32 debugCallBack(VkDebugReportFlagsEXT flags,
                       VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
                       size_t location, int32_t msgCode,
                       const char *pLayerPrefix, const char *pMsg,
                       void *pUserData);
} // namespace vividX