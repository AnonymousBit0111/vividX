#pragma once

#include "VividX/IndexBuffer.h"
#include "VividX/VertexBuffer.h"
#include "vividx.h"
#include "vulkan/vulkan_handles.hpp"
#include <memory>
#include <vector>
namespace vividX {

struct Batch {

  std::vector<PosColourVertex> vertices;
  std::vector<uint32_t> indices;
  std::unique_ptr<IndexBuffer> indexBuffer;
  std::unique_ptr<VertexBuffer> vertexBuffer;

  int vertexCount = 0;
};
} // namespace vividX