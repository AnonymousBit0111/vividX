#pragma once

#include "vividx.h"
#include <vector>
namespace vividX {

struct Batch {

  std::vector<PosColourVertex> vertices;
  std::vector<uint32_t> indices;

  int vertexCount;
};
} // namespace vividX