#include "SDL2/SDL.h"
#include "VividX/Application.h"
#include "vividx.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <cassert>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.h>

int main() {
  TracyFunction;
  ZoneScoped;
  vividX::Application app;

  app.run();

  return EXIT_SUCCESS;
}
