#pragma once

#include "glm/fwd.hpp"
#include "vividx.h"
#include <_types/_uint32_t.h>
#include <array>
#include <vector>
namespace vividX {

// The origin of the quad is its top left corner
struct Quad {

  std::vector<PosColourVertex> vertices;
  static std::array<uint32_t, 6> indices;

  Quad(glm::vec2 position,glm::vec2 size);
  ~Quad(){}
};
}; // namespace vividX