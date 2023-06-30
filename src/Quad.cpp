#include "VividX/Quad.h"
#include "glm/fwd.hpp"
#include "vividx.h"

using namespace vividX;

std::array<uint32_t, 6> Quad::indices = {0, 1, 2, 2, 3, 0};

Quad::Quad(glm::vec2 position, glm::vec2 size) {

  vertices.push_back(
      PosColourVertex{{position.x, position.y - size.y}, {1, 0,0}});
  vertices.push_back(
      PosColourVertex{{position.x + size.x, position.y - size.y}, {0, 0, 1}});
  vertices.push_back(
      PosColourVertex{{position.x + size.x, position.y}, {0, 1, 0}});

  vertices.push_back(PosColourVertex{{position.x, position.y}, {1, 1, 1}});
}