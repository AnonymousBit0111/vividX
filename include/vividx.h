#pragma once

#include "SDL2/SDL_video.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_to_string.hpp"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace vividX {

struct Vector3 {
  float x, y, z;
};
struct Vector2{
  float x,y;
};
struct PosColourVertex {
  Vector2 pos;
  Vector3 colour;
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
vk::ShaderModule createShaderModule(const std::vector<char> &code,
                                    vk::UniqueDevice &device);

inline VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}
inline VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {
  std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

  return VK_FALSE;
}
} // namespace vividX