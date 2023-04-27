#include "SDL2/SDL.h"
#include "VividX/Application.h"
#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_handles.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan.h>

int main() {

  vividX::Application app;

  app.run();

  return EXIT_SUCCESS;
}
