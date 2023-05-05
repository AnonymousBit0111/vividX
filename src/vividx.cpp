#include "vividx.h"
#include "SDL2/SDL_video.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>

#include "SDL2/SDL_vulkan.h"
#include "VkBootstrap.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"

namespace vividX {

void log(const std::string &message, Severity severity) {

  switch (severity) {

  case Severity::INFO:

    std::cout << "[INFO] " << message << "\n";
    break;
  case Severity::WARNING:

    std::cout << "[WARNING] " << message << "\n";
    break;
  case Severity::ERROR:
    std::cout << "[ERROR] " << message << "\n";
    break;

  case Severity::FATAL:
    std::cout << "[ERROR][FATAL] " << message << "\n";
    exit(-1);

    break;
  }
}

std::vector<char> readFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    assert(false && "failed to open file!");
  }
  size_t fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);
  file.seekg(0);
  file.read(buffer.data(), fileSize);
  file.close();
  return buffer;
}

vk::ShaderModule createShaderModule(const std::vector<char> &code,
                                    vk::Device &device) {
  vk::ShaderModule module;
  vk::ShaderModuleCreateInfo createInfo;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  vk::Result res = device.createShaderModule(&createInfo, nullptr, &module);

  assert(res == vk::Result::eSuccess);
  return module;
}

VkBool32 debugCallBack(VkDebugReportFlagsEXT flags,
                       VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
                       size_t location, int32_t msgCode,
                       const char *pLayerPrefix, const char *pMsg,
                       void *pUserData) {

  if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
    std::cerr << "[ERROR]: [" << pLayerPrefix << "] Code " << msgCode << " : "
              << pMsg << std::endl;
  } else if (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
    std::cerr << "[WARNING]: [" << pLayerPrefix << "] Code " << msgCode << " : "
              << pMsg << std::endl;
  }

  // exit(1);

  return VK_FALSE;
}





} // namespace vividX
